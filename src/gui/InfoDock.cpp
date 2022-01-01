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

#include <InfoDock.hpp>

#include <QGuiApplication>
#include <QMouseEvent>
#include <QScrollBar>
#include <QLabel>

#include <cmath>

void TextEdit::mouseMoveEvent(QMouseEvent *e)
{
    if (!anchorAt(e->pos()).isEmpty())
        viewport()->setProperty("cursor", QCursor(Qt::PointingHandCursor));
    else
        viewport()->setProperty("cursor", QCursor(Qt::ArrowCursor));
    QTextEdit::mouseMoveEvent(e);
}
void TextEdit::mousePressEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
    {
        QString anchor = anchorAt(e->pos());
        if (!anchor.isEmpty())
        {
            InfoDock *infoD = (InfoDock *)parent()->parent();
            if (anchor.startsWith("seek:"))
            {
                anchor.remove(0, 5);
                emit infoD->seek(anchor.toDouble());
            }
            else if (anchor.startsWith("stream:"))
            {
                anchor.remove(0, 7);
                emit infoD->chStream(anchor);
            }
            else if (anchor == "save_cover")
                emit infoD->saveCover();
        }
    }
    QTextEdit::mousePressEvent(e);
}

/**/

#include <Functions.hpp>

#include <QGridLayout>

InfoDock::InfoDock()
{
    setWindowTitle(tr("Information"));
    setVisible(false);

    setWidget(&mainW);

    infoE = new TextEdit;
    infoE->setMouseTracking(true);
    infoE->setTabChangesFocus(true);
    infoE->setUndoRedoEnabled(false);
    infoE->setLineWrapMode(QTextEdit::NoWrap);
    infoE->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoE->viewport()->setProperty("cursor", QCursor(Qt::ArrowCursor));

    buffer = new QLabel;
    bitrateAndFPS = new QLabel;

    layout = new QGridLayout(&mainW);
    layout->addWidget(infoE);
    layout->addWidget(buffer);
    layout->addWidget(bitrateAndFPS);

    QMargins margins = layout->contentsMargins();
    margins.setBottom(1);
    layout->setContentsMargins(margins);

    clear();

    connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));

    auto setInfoEditStyleSheet = [this] {
        infoE->document()->setDefaultStyleSheet("a {color: " + infoE->palette().text().color().name() + "; text-decoration: none;}");
        infoE->setHtml(m_info);
    };
    connect(qGuiApp, &QGuiApplication::paletteChanged,
            this, setInfoEditStyleSheet,
            Qt::QueuedConnection);
    setInfoEditStyleSheet();
}

void InfoDock::setInfo(const QString &info, bool _videoPlaying, bool _audioPlaying)
{
    int scroll = infoE->verticalScrollBar()->value();
    infoE->setHtml(m_info = info);
    infoE->verticalScrollBar()->setValue(scroll);
    videoPlaying = _videoPlaying;
    audioPlaying = _audioPlaying;
}
void InfoDock::updateBitrateAndFPS(int a, int v, double fps, double realFPS, bool interlaced)
{
    if (audioPlaying && a >= 0)
        audioBR = a;
    else if (!audioPlaying)
        audioBR = -1;

    if (videoPlaying)
    {
        if (v >= 0)
            videoBR = v;
        if (fps >= 0.0)
            videoFPS = fps;
        if (realFPS >= 0.0)
            videoRealFPS = realFPS;
        else if (std::isnan(realFPS))
            videoRealFPS = -1.0;
        if (v >= 0 || fps >= 0.0 || realFPS >= 0.0)
            interlacedVideo = interlaced;
    }
    else
    {
        videoBR = -1;
        videoFPS = -1.0;
        videoRealFPS = -1.0;
        interlacedVideo = false;
    }

    if (visibleRegion() != QRegion())
        setLabelValues();
}
void InfoDock::updateBuffered(qint64 backwardBytes, qint64 remainingBytes, double backwardSeconds, double remainingSeconds)
{
    if (backwardBytes < 0 || remainingBytes < 0)
    {
        if (buffer->isVisible())
        {
            buffer->clear();
            buffer->close();
        }
    }
    else
    {
        bytes1 = backwardBytes;
        bytes2 = remainingBytes;
        seconds1 = backwardSeconds;
        seconds2 = remainingSeconds;
        if (!buffer->isVisible())
            buffer->show();
        if (visibleRegion() != QRegion())
            setBufferLabel();
    }
}
void InfoDock::clear()
{
    m_info.clear();
    infoE->clear();
    videoPlaying = audioPlaying = interlacedVideo = false;
    videoBR = audioBR = -1;
    videoFPS = videoRealFPS = -1.0;
    buffer->clear();
    buffer->close();
    bitrateAndFPS->clear();
}
void InfoDock::visibilityChanged(bool v)
{
    if (v)
    {
        setLabelValues();
        if (buffer->isVisible())
            setBufferLabel();
    }
}

void InfoDock::setLabelValues()
{
    QString text;
    if (audioBR > -1)
        text += tr("Audio bitrate") + QString(": %1%2").arg(audioBR).arg(" kbps");
    if (videoBR > -1)
    {
        const bool Mbps = (videoBR >= 1024);
        if (!text.isEmpty())
            text += "\n";
        text += tr("Video bitrate") + QString(": %1%2").arg(Mbps ? videoBR / 1024.0 : videoBR, 0, Mbps ? 'f' : 'g', Mbps ? 3 : -1).arg(videoBR >= 1024 ? " Mbps" : " kbps");
    }
    if (videoFPS > -1.0)
    {
        if (!text.isEmpty())
            text += "\n";
        text += QString::number(videoFPS, 'g', 5) + " FPS";
        if (videoRealFPS >= 0.0)
            text += ", " + tr("visible") + ": " + QString::number(videoRealFPS, 'g', 5) + " FPS";
        if (interlacedVideo)
            text += ", " + tr("interlaced");
    }
    bitrateAndFPS->setText(text);
}
void InfoDock::setBufferLabel()
{
    QString txt = tr("Buffer") + ": [" + Functions::sizeString(bytes1) + ", " + Functions::sizeString(bytes2) + "]";
    if (seconds1 >= 0.0 && seconds2 >= 0.0)
        txt += ", [" + QString::number(seconds1, 'f', 1) + " s" + ", " + QString::number(seconds2, 'f', 1) + " s" + "]";
    buffer->setText(txt);
}
