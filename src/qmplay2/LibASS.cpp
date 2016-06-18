/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <QMPlay2_OSD.hpp>
#include <Settings.hpp>

#include <QColor>

extern "C"
{
	#include <ass/ass.h>
}
#include <string.h>

static void addImgs(ASS_Image *img, QMPlay2_OSD *osd)
{
	while (img)
	{
		QByteArray bitmap;
		bitmap.resize((img->w * img->h) << 2);

		quint32 *bitmap_ptr = (quint32 *)bitmap.data();

		const quint8 r = img->color >> 24;
		const quint8 g = img->color >> 16;
		const quint8 b = img->color >>  8;
		const quint8 a = ~img->color & 0xFF;

		for (int y = 0; y < img->h; y++)
		{
			const int offsetI = y * img->stride;
			const int offsetB = y * img->w;
			for (int x = 0; x < img->w; x++)
				bitmap_ptr[offsetB + x] = (a * img->bitmap[offsetI + x] / 0xFF) << 24 | b << 16 | g << 8 | r;
		}

		osd->addImage(QRect(img->dst_x, img->dst_y, img->w, img->h), bitmap);

		img = img->next;
	}
}
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

#if defined Q_OS_WIN && !defined Q_OS_WIN64
bool LibASS::slowFontCacheUpdate()
{
	return ass_library_version() < 0x01300000 || QSysInfo::windowsVersion() < QSysInfo::WV_6_0;
}
#endif

LibASS::LibASS(Settings &settings) :
	settings(settings)
{
	ass = ass_library_init();
	winW = winH = W = H = 0;
	zoom = 0.0;
	aspect_ratio = -1.0;
	fontScale = 1.0;

	osd_track = ass_sub_track = NULL;
	osd_style = NULL;
	osd_event = NULL;
	osd_renderer = ass_sub_renderer = NULL;
}
LibASS::~LibASS()
{
	closeASS();
	closeOSD();
	clearFonts();
	ass_library_done(ass);
}

void LibASS::setWindowSize(int _winW, int _winH)
{
	winW = _winW;
	winH = _winH;
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
	ass_add_font(ass, (char *)name.data(), (char *)data.data(), data.size());
}
void LibASS::clearFonts()
{
	ass_clear_fonts(ass);
	ass_set_fonts_dir(ass, NULL);
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
	ass_set_fonts(osd_renderer, NULL, NULL, 1, NULL, 1);
}
void LibASS::setOSDStyle()
{
	if (!osd_style)
		return;
	osd_style->ScaleX = osd_style->ScaleY = 1;
	readStyle("OSD", osd_style);
}
bool LibASS::getOSD(QMPlay2_OSD *&osd, const QByteArray &txt, double duration)
{
	if (!osd_track || !osd_style || !osd_event || !osd_renderer || !W || !H)
		return false;

	osd_track->PlayResX = W;
	osd_track->PlayResY = H;
	ass_set_frame_size(osd_renderer, W, H);

	osd_event->Text = (char *)txt.data();
	int ch;
	ASS_Image *img = ass_render_frame(osd_renderer, osd_track, 0, &ch);
	osd_event->Text = NULL;
	if (!img)
		return false;
	bool old_osd = osd;
	if (!old_osd)
		osd = new QMPlay2_OSD;
	else
	{
		osd->lock();
		if (ch)
			osd->clear();
	}
	osd->setText(txt);
	osd->setDuration(duration);
	if (ch || !old_osd)
	{
		addImgs(img, osd);
		osd->genChecksum();
	}
	if (old_osd)
		osd->unlock();
	osd->start();
	return true;
}
void LibASS::closeOSD()
{
	if (osd_renderer)
		ass_renderer_done(osd_renderer);
	if (osd_track)
		ass_free_track(osd_track);
	osd_track = NULL;
	osd_style = NULL;
	osd_event = NULL;
	osd_renderer = NULL;
}

void LibASS::initASS(const QByteArray &ass_data)
{
	if (ass_sub_track && ass_sub_renderer)
		return;

	ass_sub_track = ass_new_track(ass);
	if (!ass_data.isEmpty())
	{
		ass_process_data(ass_sub_track, (char *)ass_data.data(), ass_data.size());
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
	ass_set_fonts(ass_sub_renderer, NULL, NULL, true, NULL, true);
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
	if (settings.get("ApplyToASS/ApplyToASS").toBool())
	{
		colorsAndBorders = settings.get("ApplyToASS/ColorsAndBorders").toBool();
		marginsAndAlignment = settings.get("ApplyToASS/MarginsAndAlignment").toBool();
		fontsAndSpacing = settings.get("ApplyToASS/FontsAndSpacing").toBool();
		overridePlayRes = settings.get("ApplyToASS/OverridePlayRes").toBool();
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
			style->Name = NULL;
			if (ass_sub_track->styles[i].FontName)
				style->FontName = strdup(ass_sub_track->styles[i].FontName);
			else
				style->FontName = NULL;
			ass_sub_styles_copy += style;
		}
	}
	if (ass_sub_track->n_styles != ass_sub_styles_copy.size())
		return;

	for (int i = 0; i < ass_sub_track->n_styles; i++)
	{
		ASS_Style &style = ass_sub_track->styles[i];
		if (colorsAndBorders)
		{
			style.PrimaryColour = assColorFromQColor(settings.get("Subtitles/TextColor").value<QColor>());
			style.SecondaryColour = assColorFromQColor(settings.get("Subtitles/TextColor").value<QColor>(), true);
			style.OutlineColour = assColorFromQColor(settings.get("Subtitles/OutlineColor").value<QColor>());
			style.BackColour = assColorFromQColor(settings.get("Subtitles/ShadowColor").value<QColor>());
			style.BorderStyle = 1;
			style.Outline = settings.get("Subtitles/Outline").toDouble();
			style.Shadow = settings.get("Subtitles/Shadow").toDouble();
		}
		else
		{
			style.PrimaryColour = ass_sub_styles_copy[i]->PrimaryColour;
			style.SecondaryColour = ass_sub_styles_copy[i]->SecondaryColour;
			style.OutlineColour = ass_sub_styles_copy[i]->OutlineColour;
			style.BackColour = ass_sub_styles_copy[i]->BackColour;
			style.BorderStyle = ass_sub_styles_copy[i]->BorderStyle;
			style.Outline = ass_sub_styles_copy[i]->Outline;
			style.Shadow = ass_sub_styles_copy[i]->Shadow;
		}
		if (marginsAndAlignment)
		{
			style.MarginL = settings.get("Subtitles/LeftMargin").toInt();
			style.MarginR = settings.get("Subtitles/RightMargin").toInt();
			style.MarginV = settings.get("Subtitles/VMargin").toInt();
			style.Alignment = toASSAlignment(settings.get("Subtitles/Alignment").toInt());
			style.Angle = 0;
		}
		else
		{
			style.MarginL = ass_sub_styles_copy[i]->MarginL;
			style.MarginR = ass_sub_styles_copy[i]->MarginR;
			style.MarginV = ass_sub_styles_copy[i]->MarginV;
			style.Alignment = ass_sub_styles_copy[i]->Alignment;
			style.Angle = ass_sub_styles_copy[i]->Angle;
		}
		if (style.FontName)
			free(style.FontName);
		if (fontsAndSpacing)
		{
			style.FontName = strdup(settings.get("Subtitles/FontName").toString().toUtf8().data());
			style.FontSize = settings.get("Subtitles/FontSize").toInt();
			style.Spacing = settings.get("Subtitles/Linespace").toDouble();
			style.ScaleX = style.ScaleY = 1;
			style.Bold = style.Italic = style.Underline = style.StrikeOut = 0;
		}
		else
		{
			if (ass_sub_styles_copy[i]->FontName)
				style.FontName = strdup(ass_sub_styles_copy[i]->FontName);
			else
				style.FontName = NULL;
			style.FontSize = ass_sub_styles_copy[i]->FontSize;
			style.Spacing = ass_sub_styles_copy[i]->Spacing;
			style.ScaleX = ass_sub_styles_copy[i]->ScaleX;
			style.ScaleY = ass_sub_styles_copy[i]->ScaleY;
			style.Bold = ass_sub_styles_copy[i]->Bold;
			style.Italic = ass_sub_styles_copy[i]->Italic;
			style.Underline = ass_sub_styles_copy[i]->Underline;
			style.StrikeOut = ass_sub_styles_copy[i]->StrikeOut;
		}
	}
}
void LibASS::addASSEvent(const QByteArray &event)
{
	if (!ass_sub_track || !ass_sub_renderer || event.isEmpty())
		return;
	ass_process_data(ass_sub_track, (char *)event.data(), event.size());
}
void LibASS::addASSEvent(const QByteArray &text, double Start, double Duration)
{
	if (!ass_sub_track || !ass_sub_renderer || text.isEmpty() || Start < 0 || Duration < 0)
		return;
	int eventID = ass_alloc_event(ass_sub_track);
	ASS_Event *event = &ass_sub_track->events[eventID];
	event->Text = strdup(text.data());
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
bool LibASS::getASS(QMPlay2_OSD *&osd, double pos)
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
			ass_sub_track->styles[i].ScaleX  *= _fontScale;
			ass_sub_track->styles[i].ScaleY  *= _fontScale;
			ass_sub_track->styles[i].Shadow  *= _fontScale;
			ass_sub_track->styles[i].Outline *= _fontScale;
		}
	}

	ass_set_frame_size(ass_sub_renderer, W, H);
	int ch;
	ASS_Image *img = ass_render_frame(ass_sub_renderer, ass_sub_track, pos * 1000, &ch);

	if (_fontScale != 1.0)
	{
		for (int i = 0; i < ass_sub_track->n_styles; i++)
		{
			ass_sub_track->styles[i].ScaleX  /= _fontScale;
			ass_sub_track->styles[i].ScaleY  /= _fontScale;
			ass_sub_track->styles[i].Shadow  /= _fontScale;
			ass_sub_track->styles[i].Outline /= _fontScale;
		}
	}

	if (overridePlayRes)
	{
		ass_sub_track->PlayResX = playResX;
		ass_sub_track->PlayResY = playResY;
	}

	if (!img)
		return false;
	bool old_osd = osd;
	if (!old_osd)
		osd = new QMPlay2_OSD;
	else
	{
		osd->lock();
		if (ch)
			osd->clear(false);
	}
	osd->setPTS(pos);
	if (ch || !old_osd)
	{
		addImgs(img, osd);
		osd->genChecksum();
	}
	if (old_osd)
		osd->unlock();
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
	ass_sub_track = NULL;
	ass_sub_renderer = NULL;
}

void LibASS::readStyle(const QString &prefix, ASS_Style *style)
{
	if (style->FontName)
		free(style->FontName);
	style->FontName = strdup(settings.get(prefix + "/FontName").toString().toUtf8().data());
	style->FontSize = settings.get(prefix + "/FontSize").toInt();
	style->PrimaryColour = style->SecondaryColour = assColorFromQColor(settings.get(prefix + "/TextColor").value<QColor>());
	style->OutlineColour = assColorFromQColor(settings.get(prefix + "/OutlineColor").value<QColor>());
	style->BackColour = assColorFromQColor(settings.get(prefix + "/ShadowColor").value<QColor>());
	style->Spacing = settings.get(prefix + "/Linespace").toDouble();
	style->BorderStyle = 1;
	style->Outline = settings.get(prefix + "/Outline").toDouble();
	style->Shadow = settings.get(prefix + "/Shadow").toDouble();
	style->Alignment = toASSAlignment(settings.get(prefix + "/Alignment").toInt());
	style->MarginL = settings.get(prefix + "/LeftMargin").toInt();
	style->MarginR = settings.get(prefix + "/RightMargin").toInt();
	style->MarginV = settings.get(prefix + "/VMargin").toInt();
}
void LibASS::calcSize()
{
	if (!winW || !winH || zoom <= 0.0 || aspect_ratio < 0.0)
		return;
	W = winW;
	H = winH;
	if (aspect_ratio > 0.0)
	{
		if (winW / aspect_ratio > winH)
			W = H * aspect_ratio;
		else
			H = W / aspect_ratio;
	}
	W *= zoom;
	H *= zoom;
}
