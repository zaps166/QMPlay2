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

#include <QTextEdit>

#include <DockWidget.hpp>

class TextEdit final : public QTextEdit
{
private:
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
};

/**/

class QGridLayout;
class QLabel;

class InfoDock : public DockWidget
{
    friend class TextEdit;
    Q_OBJECT
public:
    InfoDock();
public slots:
    void setInfo(const QString &, bool, bool);
    void updateBitrateAndFPS(int a, int v, double fps, double realFPS, bool interlaced);
    void updateBuffered(qint64 backwardBytes, qint64 remainingBytes, double backwardSeconds, double remainingSeconds);
    void clear();
    void visibilityChanged(bool);
private:
    void setLabelValues();
    void setBufferLabel();

    QWidget mainW;
    QGridLayout *layout;
    QLabel *bitrateAndFPS, *buffer;
    TextEdit *infoE;

    QString m_info;

    bool videoPlaying, audioPlaying, interlacedVideo;
    int audioBR, videoBR;
    double videoFPS, videoRealFPS;
    qint64 bytes1, bytes2;
    double seconds1, seconds2;
signals:
    void seek(double pos);
    void chStream(const QString &);
    void saveCover();
};
