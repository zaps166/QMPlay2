// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VisualizationsHeaders.hpp>

#include <QMPlay2Extensions.hpp>
#include <VisWidget.hpp>

class SimpleVis;

class SimpleVisW final : public VisWidget
{
    friend class SimpleVis;

    Q_DECLARE_TR_FUNCTIONS(SimpleVisW)
public:
    SimpleVisW(SimpleVis &);
private:
    void paint(QPainter &p) override;

    void resizeEvent(QResizeEvent *) override;

    void start() override;
    void stop() override;

    QByteArray soundData;
    quint8 chn;
    quint32 srate;
    int interval;
    qreal leftBar, rightBar;
    QPair<qreal, double> leftLine, rightLine;
    SimpleVis &simpleVis;
    QLinearGradient linearGrad;
    bool fullScreen;
};

/**/

class SimpleVis final : public QMPlay2Extensions
{
public:
    SimpleVis(Module &);

    void soundBuffer(const bool);

    bool set() override;
private:
    DockWidget *getDockWidget() override;

    bool isVisualization() const override;
    void connectDoubleClick(const QObject *, const char *) override;
    void visState(bool, uchar, uint) override;
    void sendSoundData(const QByteArray &) override;
    void clearSoundData() override;

    /**/

    SimpleVisW w;

    QByteArray tmpData;
    int tmpDataPos;
    QMutex mutex;
    float sndLen;
};

#define SimpleVisName "Prosta wizualizacja"
