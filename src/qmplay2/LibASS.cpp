/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <LibASS.hpp>

#ifdef QMPLAY2_LIBASS

#include <QMPlay2OSD.hpp>
#include <Functions.hpp>
#include <Settings.hpp>

#include <QColor>

extern "C" {
#include <ass/ass.h>
}

#include <cstring>

using namespace std;

#ifdef USE_VULKAN
#   include "../qmvk/PhysicalDevice.hpp"
#   include "../qmvk/Device.hpp"
#   include "../qmvk/Buffer.hpp"
#   include "../qmvk/BufferView.hpp"

#   include <vulkan/VulkanInstance.hpp>
#   include <vulkan/VulkanBufferPool.hpp>
#endif

static inline quint32 assColorFromQColor(const QColor &color, bool invert = false)
{
    if (!invert)
        return color.red() << 24 | color.green() << 16 | color.blue() << 8 | (~color.alpha() & 0xFF);
    return (~color.red() & 0xFF) << 24 | (~color.green() & 0xFF) << 16 | (~color.blue() & 0xFF) << 8 | (~color.alpha() & 0xFF);
}
static inline int toASSAlignment(int align)
{
    switch (align)
    {
        case 0:
            return 5;
        case 1:
            return 6;
        case 2:
            return 7;
        case 3:
            return 9;
        case 4:
            return 10;
        case 5:
            return 11;
        case 6:
            return 1;
        case 7:
            return 2;
        case 8:
            return 3;
        default:
            return 0;
    }
}

/**/

bool LibASS::isDummy()
{
    return false;
}

LibASS::LibASS(Settings &settings) :
    settings(settings)
{
    ass = ass_library_init();
    winW = winH = W = H = 0;
    zoom = 0.0;
    aspect_ratio = -1.0;
    fontScale = 1.0;

    osd_track = ass_sub_track = nullptr;
    osd_style = nullptr;
    osd_event = nullptr;
    osd_renderer = ass_sub_renderer = nullptr;

#ifdef USE_VULKAN
    if (QMPlay2Core.isVulkanRenderer())
        m_vkBufferPool = std::static_pointer_cast<QmVk::Instance>(QMPlay2Core.gpuInstance())->createBufferPool();
#endif
}
LibASS::~LibASS()
{
    closeASS();
    closeOSD();
    clearFonts();
    ass_library_done(ass);
}

void LibASS::addImgs(ass_image *img, QMPlay2OSD *osd)
{
#ifdef USE_VULKAN
    if (m_vkBufferPool)
    {
        using namespace QmVk;

        auto device = m_vkBufferPool->instance()->device();
        if (!device)
            return;

        const auto alignment = device->physicalDevice()->limits().minTexelBufferOffsetAlignment;

        auto getImgBuffSize = [&](ASS_Image *img) {
            return FFALIGN(img->stride * img->h, alignment);
        };

        auto imgBegin = img;

        vk::DeviceSize buffSize = 0;
        vk::DeviceSize buffOffset = 0;

        img = imgBegin;
        while (img)
        {
            buffSize += getImgBuffSize(img);
            img = img->next;
        }

        auto buffer = m_vkBufferPool->take(buffSize);
        if (!buffer)
            return;

        auto data = buffer->map<uint8_t>();

        img = imgBegin;
        while (img) try
        {
            if (img->w <= 0 || img->h <= 0)
            {
                img = img->next;
                continue;
            }

            const vk::DeviceSize viewSize = getImgBuffSize(img);
            const uint32_t imgSize = img->stride * (img->h - 1) + img->w;
            memcpy(data + buffOffset, img->bitmap, imgSize);

            auto &osdImg = osd->add();
            osdImg.rect = QRect(img->dst_x, img->dst_y, img->w, img->h);
            osdImg.dataBufferView = BufferView::create(buffer, vk::Format::eR8Unorm, buffOffset, imgSize);
            osdImg.linesize = img->stride;
            osdImg.color.setX(((img->color >> 24) & 0xff) / 255.0f);
            osdImg.color.setY(((img->color >> 16) & 0xff) / 255.0f);
            osdImg.color.setZ(((img->color >>  8) & 0xff) / 255.0f);
            osdImg.color.setW(((~img->color)      & 0xff) / 255.0f);

            buffOffset += viewSize;

            img = img->next;
        }
        catch (const vk::SystemError &e)
        {
            Q_UNUSED(e)
            osd->clear();
            return;
        }

        osd->genId();
        osd->setReturnVkBufferFn(m_vkBufferPool, move(buffer));

        return;
    }
#endif

    while (img)
    {
        auto &osdImg = osd->add();
        osdImg.rect = QRect(img->dst_x, img->dst_y, img->w, img->h);
        osdImg.rgba = QByteArray(img->w * img->h * sizeof(uint32_t), Qt::Uninitialized);

        const quint8 r = img->color >> 24;
        const quint8 g = img->color >> 16;
        const quint8 b = img->color >>  8;
        const quint8 a = ~img->color & 0xFF;

        auto data = reinterpret_cast<uint32_t *>(osdImg.rgba.data());
        for (int y = 0; y < img->h; y++)
        {
            const int offsetI = y * img->stride;
            const int offsetB = y * img->w;
            for (int x = 0; x < img->w; x++)
                data[offsetB + x] = (a * img->bitmap[offsetI + x] / 0xFF) << 24 | b << 16 | g << 8 | r;
        }

        img = img->next;
    }

    osd->genId();
}

void LibASS::setWindowSize(int _winW, int _winH)
{
    const qreal dpr = QMPlay2Core.getVideoDevicePixelRatio();
    winW = _winW * dpr;
    winH = _winH * dpr;
    calcSize();
}
void LibASS::setARatio(double _aspect_ratio)
{
    aspect_ratio = _aspect_ratio;
    calcSize();
}
void LibASS::setZoom(double _zoom)
{
    zoom = _zoom;
    calcSize();
}
void LibASS::setFontScale(double fs)
{
    fontScale = fs;
}

void LibASS::addFont(const QByteArray &name, const QByteArray &data)
{
    ass_add_font(ass, (char *)name.constData(), (char *)data.constData(), data.size());
}
void LibASS::clearFonts()
{
    ass_clear_fonts(ass);
    ass_set_fonts_dir(ass, nullptr);
}

void LibASS::initOSD()
{
    if (osd_track && osd_style && osd_event && osd_renderer)
        return;

    osd_track = ass_new_track(ass);

    int styleID = ass_alloc_style(osd_track);
    osd_style = &osd_track->styles[styleID];
    setOSDStyle();

    int eventID = ass_alloc_event(osd_track);
    osd_event = &osd_track->events[eventID];
    osd_event->Start = 0;
    osd_event->Duration = 1;
    osd_event->Style = styleID;
    osd_event->ReadOrder = eventID;

    osd_renderer = ass_renderer_init(ass);
    ass_set_fonts(osd_renderer, nullptr, nullptr, 1, nullptr, 1);
}
void LibASS::setOSDStyle()
{
    if (!osd_style)
        return;
    osd_style->ScaleX = osd_style->ScaleY = 1;
    readStyle("OSD", osd_style);
}
bool LibASS::getOSD(QMPlay2OSD *&osd, const QByteArray &txt, double duration)
{
    if (!osd_track || !osd_style || !osd_event || !osd_renderer || !W || !H)
        return false;

    const qreal dpr = QMPlay2Core.getVideoDevicePixelRatio();
    osd_track->PlayResX = W / dpr;
    osd_track->PlayResY = H / dpr;
    ass_set_frame_size(osd_renderer, W, H);

    osd_event->Text = (char *)txt.constData();
    int ch;
    ASS_Image *img = ass_render_frame(osd_renderer, osd_track, 0, &ch);
    osd_event->Text = nullptr;
    if (!img)
        return false;
    unique_lock<mutex> locker;
    if (!osd)
    {
        osd = new QMPlay2OSD;
    }
    else
    {
        locker = osd->lock();
        if (ch)
            osd->clear();
    }
    osd->setText(txt);
    osd->setDuration(duration);
    if (ch || !locker.owns_lock())
        addImgs(img, osd);
    osd->start();
    return true;
}
void LibASS::closeOSD()
{
    if (osd_renderer)
        ass_renderer_done(osd_renderer);
    if (osd_track)
        ass_free_track(osd_track);
    osd_track = nullptr;
    osd_style = nullptr;
    osd_event = nullptr;
    osd_renderer = nullptr;
}

void LibASS::initASS(const QByteArray &ass_data)
{
    if (ass_sub_track && ass_sub_renderer)
        return;

    ass_sub_track = ass_new_track(ass);
    if (!ass_data.isEmpty())
    {
        ass_process_data(ass_sub_track, (char *)ass_data.constData(), ass_data.size());
        hasASSData = true;
        setASSStyle();
    }
    else
    {
        ass_alloc_style(ass_sub_track);
        ass_sub_track->styles[0].ScaleX = ass_sub_track->styles[0].ScaleY = 1;
        overridePlayRes = true;
        hasASSData = false;
        setASSStyle();
    }

    ass_sub_renderer = ass_renderer_init(ass);
    ass_set_fonts(ass_sub_renderer, nullptr, nullptr, true, nullptr, true);
}
bool LibASS::isASS() const
{
    return hasASSData && ass_sub_track && ass_sub_renderer;
}
void LibASS::setASSStyle()
{
    if (!ass_sub_track)
        return;

    if (!hasASSData)
    {
        readStyle("Subtitles", &ass_sub_track->styles[0]);
        return;
    }

    bool colorsAndBorders, marginsAndAlignment, fontsAndSpacing;
    if (settings.getBool("ApplyToASS/ApplyToASS"))
    {
        colorsAndBorders = settings.getBool("ApplyToASS/ColorsAndBorders");
        marginsAndAlignment = settings.getBool("ApplyToASS/MarginsAndAlignment");
        fontsAndSpacing = settings.getBool("ApplyToASS/FontsAndSpacing");
        overridePlayRes = settings.getBool("ApplyToASS/OverridePlayRes");
    }
    else
        colorsAndBorders = marginsAndAlignment = fontsAndSpacing = overridePlayRes = false;

    if (!ass_sub_styles_copy.size()) //tworzenie kopii za pierwszym razem
    {
        if (!colorsAndBorders && !marginsAndAlignment && !fontsAndSpacing) //nie ma nic do zastosowania
            return;
        for (int i = 0; i < ass_sub_track->n_styles; i++)
        {
            ASS_Style *style = new ASS_Style;
            memcpy(style, &ass_sub_track->styles[i], sizeof(ASS_Style));
            style->Name = nullptr;
            if (ass_sub_track->styles[i].FontName)
                style->FontName = strdup(ass_sub_track->styles[i].FontName);
            else
                style->FontName = nullptr;
            ass_sub_styles_copy += style;
        }
    }
    if (ass_sub_track->n_styles != ass_sub_styles_copy.size())
        return;

    for (int i = 0; i < ass_sub_track->n_styles; i++)
    {
        ASS_Style &style = ass_sub_track->styles[i];
        ass_style *style_copy = ass_sub_styles_copy.at(i);
        if (colorsAndBorders)
        {
            style.PrimaryColour = assColorFromQColor(settings.getColor("Subtitles/TextColor"));
            style.SecondaryColour = assColorFromQColor(settings.getColor("Subtitles/TextColor"), true);
            style.OutlineColour = assColorFromQColor(settings.getColor("Subtitles/OutlineColor"));
            style.BackColour = assColorFromQColor(settings.getColor("Subtitles/ShadowColor"));
            style.BorderStyle = 1;
            style.Outline = settings.getDouble("Subtitles/Outline");
            style.Shadow = settings.getDouble("Subtitles/Shadow");
        }
        else
        {
            style.PrimaryColour = style_copy->PrimaryColour;
            style.SecondaryColour = style_copy->SecondaryColour;
            style.OutlineColour = style_copy->OutlineColour;
            style.BackColour = style_copy->BackColour;
            style.BorderStyle = style_copy->BorderStyle;
            style.Outline = style_copy->Outline;
            style.Shadow = style_copy->Shadow;
        }
        if (marginsAndAlignment)
        {
            style.MarginL = settings.getInt("Subtitles/LeftMargin");
            style.MarginR = settings.getInt("Subtitles/RightMargin");
            style.MarginV = settings.getInt("Subtitles/VMargin");
            style.Alignment = toASSAlignment(settings.getInt("Subtitles/Alignment"));
            style.Angle = 0;
        }
        else
        {
            style.MarginL = style_copy->MarginL;
            style.MarginR = style_copy->MarginR;
            style.MarginV = style_copy->MarginV;
            style.Alignment = style_copy->Alignment;
            style.Angle = style_copy->Angle;
        }
        if (style.FontName)
            free(style.FontName);
        if (fontsAndSpacing)
        {
            style.FontName = strdup(settings.getString("Subtitles/FontName").toUtf8().constData());
            style.FontSize = settings.getInt("Subtitles/FontSize");
            style.Spacing = settings.getDouble("Subtitles/Linespace");
            style.ScaleX = style.ScaleY = 1;
            style.Bold = style.Italic = style.Underline = style.StrikeOut = 0;
        }
        else
        {
            if (style_copy->FontName)
                style.FontName = strdup(style_copy->FontName);
            else
                style.FontName = nullptr;
            style.FontSize = style_copy->FontSize;
            style.Spacing = style_copy->Spacing;
            style.ScaleX = style_copy->ScaleX;
            style.ScaleY = style_copy->ScaleY;
            style.Bold = style_copy->Bold;
            style.Italic = style_copy->Italic;
            style.Underline = style_copy->Underline;
            style.StrikeOut = style_copy->StrikeOut;
        }
    }
}
void LibASS::addASSEvent(const QByteArray &event)
{
    if (!ass_sub_track || !ass_sub_renderer || event.isEmpty())
        return;
    ass_process_data(ass_sub_track, (char *)event.constData(), event.size());
}
void LibASS::addASSEvent(const QByteArray &text, double Start, double Duration)
{
    if (!ass_sub_track || !ass_sub_renderer || text.isEmpty() || Start < 0 || Duration < 0)
        return;
    int eventID = ass_alloc_event(ass_sub_track);
    ASS_Event *event = &ass_sub_track->events[eventID];
    event->Text = strdup(text.constData());
    event->Start = Start * 1000;
    event->Duration = Duration * 1000;
    event->Style = 0;
    event->ReadOrder = eventID;
}
void LibASS::flushASSEvents()
{
    if (!ass_sub_track || !ass_sub_renderer)
        return;
    ass_flush_events(ass_sub_track);
}
bool LibASS::getASS(QMPlay2OSD *&osd, double pos)
{
    if (!ass_sub_track || !ass_sub_renderer || !W || !H)
        return false;

    int playResX = ass_sub_track->PlayResX;
    int playResY = ass_sub_track->PlayResY;
    if (overridePlayRes)
    {
        ass_sub_track->PlayResX = 384;
        ass_sub_track->PlayResY = 288;
    }

    double _fontScale = fontScale;

    if (_fontScale != 1.0)
    {
        for (int i = 0; i < ass_sub_track->n_styles; i++)
        {
            ASS_Style &style = ass_sub_track->styles[i];
            style.ScaleX  *= _fontScale;
            style.ScaleY  *= _fontScale;
            style.Shadow  *= _fontScale;
            style.Outline *= _fontScale;
        }
    }

    ass_set_frame_size(ass_sub_renderer, W, H);

    const int marginLR = qMax(0, W / 2 - winW / 2);
    const int marginTB = qMax(0, H / 2 - winH / 2);
    ass_set_margins(ass_sub_renderer, marginTB, marginTB, marginLR, marginLR);

    int ch;
    ASS_Image *img = ass_render_frame(ass_sub_renderer, ass_sub_track, pos * 1000, &ch);

    if (_fontScale != 1.0)
    {
        for (int i = 0; i < ass_sub_track->n_styles; i++)
        {
            ASS_Style &style = ass_sub_track->styles[i];
            style.ScaleX  /= _fontScale;
            style.ScaleY  /= _fontScale;
            style.Shadow  /= _fontScale;
            style.Outline /= _fontScale;
        }
    }

    if (overridePlayRes)
    {
        ass_sub_track->PlayResX = playResX;
        ass_sub_track->PlayResY = playResY;
    }

    if (!img)
        return false;

    unique_lock<mutex> locker;

    if (!osd)
    {
        osd = new QMPlay2OSD;
    }
    else
    {
        locker = osd->lock();
        if (ch)
            osd->clear();
    }
    osd->setPTS(pos);
    if (ch || !locker.owns_lock())
        addImgs(img, osd);
    return true;
}
void LibASS::closeASS()
{
    while (ass_sub_styles_copy.size())
    {
        ASS_Style *style = ass_sub_styles_copy.takeFirst();
        if (style->FontName)
            free(style->FontName);
        delete style;
    }
    if (ass_sub_renderer)
        ass_renderer_done(ass_sub_renderer);
    if (ass_sub_track)
        ass_free_track(ass_sub_track);
    ass_sub_track = nullptr;
    ass_sub_renderer = nullptr;
}

void LibASS::readStyle(const QString &prefix, ASS_Style *style)
{
    if (style->FontName)
        free(style->FontName);
    style->FontName = strdup(settings.getString(prefix + "/FontName").toUtf8().constData());
    style->FontSize = settings.getInt(prefix + "/FontSize");
    style->PrimaryColour = style->SecondaryColour = assColorFromQColor(settings.getColor(prefix + "/TextColor"));
    style->OutlineColour = assColorFromQColor(settings.getColor(prefix + "/OutlineColor"));
    style->BackColour = assColorFromQColor(settings.getColor(prefix + "/ShadowColor"));
    style->Spacing = settings.getDouble(prefix + "/Linespace");
    style->BorderStyle = 1;
    style->Outline = settings.getDouble(prefix + "/Outline");
    style->Shadow = settings.getDouble(prefix + "/Shadow");
    style->Alignment = toASSAlignment(settings.getInt(prefix + "/Alignment"));
    style->MarginL = settings.getInt(prefix + "/LeftMargin");
    style->MarginR = settings.getInt(prefix + "/RightMargin");
    style->MarginV = settings.getInt(prefix + "/VMargin");
}
inline void LibASS::calcSize()
{
    Functions::getImageSize(aspect_ratio, zoom, winW, winH, W, H);
}

#else // QMPLAY2_LIBASS

bool LibASS::isDummy()
{
    return true;
}

LibASS::LibASS(Settings &settings) :
    settings(settings)
{}
LibASS::~LibASS()
{}

void LibASS::addImgs(ass_image *, QMPlay2OSD *)
{}

void LibASS::setWindowSize(int, int)
{}
void LibASS::setARatio(double)
{}
void LibASS::setZoom(double)
{}
void LibASS::setFontScale(double)
{}

void LibASS::addFont(const QByteArray &, const QByteArray &)
{}
void LibASS::clearFonts()
{}

void LibASS::initOSD()
{}
void LibASS::setOSDStyle()
{}
bool LibASS::getOSD(QMPlay2OSD *&, const QByteArray &, double)
{
    return false;
}
void LibASS::closeOSD()
{}

void LibASS::initASS(const QByteArray &)
{}
bool LibASS::isASS() const
{
    return false;
}
void LibASS::setASSStyle()
{}
void LibASS::addASSEvent(const QByteArray &)
{}
void LibASS::addASSEvent(const QByteArray &, double, double)
{}
void LibASS::flushASSEvents()
{}
bool LibASS::getASS(QMPlay2OSD *&, double)
{
    return false;
}
void LibASS::closeASS()
{}

#endif // QMPLAY2_LIBASS
