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

#pragma once

#include <QString>
#include <QVector>
#include <QMutex>

class QRect;
class Frame;
class QMPlay2OSD;
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

    void draw(const Frame &, const QRect &, const QRect &, int, int, const QList<const QMPlay2OSD *> &, QMutex &);
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
