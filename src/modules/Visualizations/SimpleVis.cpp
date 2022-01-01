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

#include <SimpleVis.hpp>
#include <Functions.hpp>

#include <QPainter>
#include <QPainterPath>

#include <cmath>

static inline float fltclip(float f)
{
    if (f > 1.0f)
        f = 1.0f;
    else if (f < -1.0f)
        f = -1.0f;
    else if (f != f) //NaN
        f = 0.0f;
    return f;
}
static inline void fltcpy(float *dest, const float *src, int size)
{
    size /= sizeof(float);
    for (int i = 0; i < size; ++i)
        dest[i] = fltclip(src[i]);
}

/**/

SimpleVisW::SimpleVisW(SimpleVis &simpleVis) :
    simpleVis(simpleVis),
    fullScreen(false)
{
    dw->setObjectName(SimpleVisName);
    dw->setWindowTitle(tr("Simple visualization"));
    dw->setWidget(this);

    chn = 2;
    srate = 0;
    leftBar = rightBar = leftLine.first = rightLine.first = 0.0;
    interval = -1;

    linearGrad.setFinalStop(0.0, 0.0);
    linearGrad.setColorAt(0.0, Qt::blue);
    linearGrad.setColorAt(0.1, Qt::green);
    linearGrad.setColorAt(0.5, Qt::yellow);
    linearGrad.setColorAt(0.8, Qt::red);
}

void SimpleVisW::paint(QPainter &p)
{
    const int size = soundData.size() / sizeof(float);
    if (size >= chn)
    {
        const float *samples = (const float *)soundData.constData();
        const qreal dpr = devicePixelRatioF();

        qreal lr[2] = {0.0f, 0.0f};

        QTransform t;
        t.translate(0.0, fullScreen);
        t.scale((width() - 1) * 0.9, (height() - 1 - fullScreen) / 2.0 / chn);
        t.translate(0.055, 0.0);
        for (int c = 0; c < chn; ++c)
        {
            p.setPen(QColor(102, 51, 128));
            p.drawLine(t.map(QLineF(0.0, 1.0, 1.0, 1.0)));

            p.setPen(QPen(QColor(102, 179, 102), 1.0 / dpr));
            QPainterPath path(t.map(QPointF(0.0, 1.0 - samples[c])));
            for (int i = chn; i < size; i += chn)
                path.lineTo(t.map(QPointF(i / (qreal)(size - chn), 1.0 - samples[i + c])));
            p.drawPath(path);

            if (c < 2)
            {
                const int numSamples = size / chn;
                for (int i = 0; i < size; i += chn)
                    lr[c] += samples[i + c] * samples[i + c];
                lr[c] = 20.0 * log10(sqrt(lr[c] / numSamples));
                lr[c] = qMin<qreal>(qMax<qreal>(0.0, lr[c] + 43.0) / 40.0, 1.0);
            }

            t.translate(0.0, 2.0);
        }

        t.reset();
        t.scale(width()-1, height()-1);

        linearGrad.setStart(t.map(QPointF(0.0, 1.0)));

        if (chn == 1)
            lr[1] = lr[0];

        const double currTime = Functions::gettime();
        const double realInterval = currTime - time;
        time = currTime;

        /* Bars */
        setValue(leftBar,  lr[0], realInterval * 2.0);
        setValue(rightBar, lr[1], realInterval * 2.0);
        p.fillRect(t.mapRect(QRectF(0.005, 1.0, 0.035, -leftBar )), linearGrad);
        p.fillRect(t.mapRect(QRectF(0.960, 1.0, 0.035, -rightBar)), linearGrad);

        /* Horizontal lines over side bars */
        setValue(leftLine,  lr[0], realInterval * 0.5);
        setValue(rightLine, lr[1], realInterval * 0.5);
        p.setPen(QPen(linearGrad, 1.0));
        p.drawLine(t.map(QLineF(0.005, -leftLine.first  + 1.0, 0.040, -leftLine.first  + 1.0)));
        p.drawLine(t.map(QLineF(0.960, -rightLine.first + 1.0, 0.995, -rightLine.first + 1.0)));

        if (stopped && tim.isActive() && leftLine.first == lr[0] && rightLine.first == lr[1])
            tim.stop();
    }
}

void SimpleVisW::resizeEvent(QResizeEvent *e)
{
    fullScreen = window()->property("fullScreen").toBool();
    VisWidget::resizeEvent(e);
}

void SimpleVisW::start()
{
    if (canStart())
    {
        simpleVis.soundBuffer(true);
        tim.start(interval);
        time = Functions::gettime();
    }
}
void SimpleVisW::stop()
{
    tim.stop();
    simpleVis.soundBuffer(false);
    leftBar = rightBar = leftLine.first = rightLine.first = 0.0f;
    VisWidget::stop();
}

/**/

SimpleVis::SimpleVis(Module &module) :
    w(*this), tmpDataPos(0)
{
    SetModule(module);
}

void SimpleVis::soundBuffer(const bool enable)
{
    QMutexLocker mL(&mutex);
    const int arrSize = enable ? (ceil(sndLen * w.srate) * w.chn * sizeof(float)) : 0;
    if (arrSize != tmpData.size() || arrSize != w.soundData.size())
    {
        tmpDataPos = 0;
        tmpData.clear();
        if (arrSize)
        {
            tmpData.resize(arrSize);
            const int oldSize = w.soundData.size();
            w.soundData.resize(arrSize);
            if (arrSize > oldSize)
                memset(w.soundData.data() + oldSize, 0, arrSize - oldSize);
        }
        else
            w.soundData.clear();
    }
}

bool SimpleVis::set()
{
    const bool isGlOnWindow = QMPlay2Core.isGlOnWindow();
    w.setUseOpenGL(isGlOnWindow);
    w.interval = isGlOnWindow ? 1 : sets().getInt("RefreshTime");
    sndLen = sets().getInt("SimpleVis/SoundLength") / 1000.0f;
    if (w.tim.isActive())
        w.start();
    return true;
}

DockWidget *SimpleVis::getDockWidget()
{
    return w.dw;
}

bool SimpleVis::isVisualization() const
{
    return true;
}
void SimpleVis::connectDoubleClick(const QObject *receiver, const char *method)
{
    w.connect(&w, SIGNAL(doubleClicked()), receiver, method);
}
void SimpleVis::visState(bool playing, uchar chn, uint srate)
{
    if (playing)
    {
        if (!chn || !srate)
            return;
        w.chn = chn;
        w.srate = srate;
        w.stopped = false;
        w.start();
    }
    else
    {
        if (!chn && !srate)
        {
            w.srate = 0;
            w.stop();
        }
        w.stopped = true;
        w.update();
    }
}
void SimpleVis::sendSoundData(const QByteArray &data)
{
    if (!w.tim.isActive() || !data.size())
        return;
    QMutexLocker mL(&mutex);
    if (!tmpData.size())
        return;
    int newDataPos = 0;
    while (newDataPos < data.size())
    {
        const int size = qMin(data.size() - newDataPos, tmpData.size() - tmpDataPos);
        fltcpy((float *)(tmpData.data() + tmpDataPos), (const float *)(data.constData() + newDataPos), size);
        newDataPos += size;
        tmpDataPos += size;
        if (tmpDataPos == tmpData.size())
        {
            memcpy(w.soundData.data(), tmpData.constData(), tmpDataPos);
            tmpDataPos = 0;
        }
    }
}
void SimpleVis::clearSoundData()
{
    if (w.tim.isActive())
    {
        QMutexLocker mL(&mutex);
        w.soundData.fill(0);
        w.stopped = true;
        w.update();
    }
}
