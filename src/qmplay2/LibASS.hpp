// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

class Settings;
class QMPlay2OSD;
struct ass_style;
struct ass_library;
struct ass_track;
struct ass_event;
struct ass_renderer;
struct ass_image;

#ifdef USE_VULKAN
namespace QmVk {
class BufferPool;
}
#endif

class QMPLAY2SHAREDLIB_EXPORT LibASS
{
public:
    static bool isDummy();

    LibASS(Settings &);
    ~LibASS();

    bool addImgs(ass_image *img, QMPlay2OSD *osd);

    void setWindowSize(const QSize &winSize);
    void setARatio(double);
    void setZoom(double);
    void setFontScale(double);

    void addFont(const QByteArray &name, const QByteArray &data);

    void initOSD();
    void setOSDStyle();
    bool getOSD(std::shared_ptr<QMPlay2OSD> &osd, const QByteArray &, double);
    void closeOSD();

    void initASS(const QByteArray &header = QByteArray());
    bool isASS() const;
    void setASSStyle();
    void addASSEvent(const QByteArray &);
    void addASSEvents(const QList<QByteArray> &events, double start, double duration);
    void addASSEvent(const QByteArray &, double, double);
    void flushASSEvents();
    bool getASS(std::shared_ptr<QMPlay2OSD> &osd, double pos = qQNaN());
    void closeASS();

private:
    void readStyle(const QString &, ass_style *);
    inline void calcSize();

private:
    Settings &settings;

    ass_library *m_osdAss;
    int W, H, winW, winH;
    double zoom, aspect_ratio, fontScale;

    //OSD
    ass_track *osd_track;
    ass_style *osd_style;
    ass_event *osd_event;
    ass_renderer *osd_renderer;

    //ASS subtitles
    ass_library *m_subsAss;
    ass_track *ass_sub_track;
    ass_renderer *ass_sub_renderer;
    QList<ass_style *> ass_sub_styles_copy;
    bool hasASSData;
    double m_lastPos = qQNaN();
    std::set<int> m_assIDs;

#ifdef USE_VULKAN
    std::shared_ptr<QmVk::BufferPool> m_vkBufferPool;
#endif
};
