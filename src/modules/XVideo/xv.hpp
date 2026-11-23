// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <XVideoHeaders.hpp>

#include <QMPlay2OSD.hpp>

class Frame;
struct XVideoPrivate;

class XVIDEO
{
public:
    XVIDEO();
    ~XVIDEO();

    inline bool isOK()
    {
        return _isOK;
    }
    inline bool isOpen()
    {
        return _isOpen;
    }

    void open(int, int, unsigned long, const QString &adaptorName, bool);

    void freeImage();
    void invalidateShm();
    void close();

    void draw(const Frame &, const QRect &, const QRect &, int, int, const QMPlay2OSDList &osd_list);
    void redraw(const QRect &, const QRect &, int, int, int, int, int, int);

    void setVideoEqualizer(int, int, int, int);
    void setFlip(int);
    inline int flip()
    {
        return _flip;
    }

    static QStringList adaptorsList();
private:
    void putImage(const QRect &srcRect, const QRect &dstRect);

    void XvSetPortAttributeIfExists(void *attributes, int attrib_count, const char *k, int v);
    void clrVars();

    bool _isOK, _isOpen, hasImage;
    int _flip;

    unsigned long handle;
    int width, height;

    unsigned int adaptors;

    QVector<quint64> osd_ids;

    XVideoPrivate *priv;
};
