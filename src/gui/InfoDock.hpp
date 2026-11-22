// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
