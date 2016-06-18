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

#include <InfoDock.hpp>

#include <QMouseEvent>
#include <QScrollBar>
#include <QLabel>

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
			if (anchor.left(5) == "seek:")
			{
				anchor.remove(0, 5);
				emit infoD->seek(anchor.toDouble());
			}
			else if (anchor.left(7) == "stream:")
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
	bitrate = new QLabel;

	layout = new QGridLayout(&mainW);
	layout->addWidget(infoE);
	layout->addWidget(buffer);
	layout->addWidget(bitrate);

	clear();

	connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
}

void InfoDock::setInfo(const QString &info, bool _videoPlaying, bool _audioPlaying)
{
	int scroll = infoE->verticalScrollBar()->value();
	infoE->setHtml(info);
	infoE->verticalScrollBar()->setValue(scroll);
	videoPlaying = _videoPlaying;
	audioPlaying = _audioPlaying;
}
void InfoDock::updateBitrate(int a, int v, double fps)
{
	if (audioPlaying && a >= 0)
		audioBR = a;
	else if (!audioPlaying)
		audioBR = -1;

	if (videoPlaying && v >= 0)
		videoBR = v;
	else if (!videoPlaying)
		videoBR = -1;

	if (videoPlaying && fps >= 0.0)
		videoFPS = fps;
	else if (!videoPlaying)
		videoFPS = -1.0;

	if (visibleRegion() != QRegion())
		setBRLabels();
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
	infoE->clear();
	videoPlaying = audioPlaying = false;
	videoBR = audioBR = -1;
	videoFPS = -1.0;
	buffer->clear();
	buffer->close();
	bitrate->setText("\n");
}
void InfoDock::visibilityChanged(bool v)
{
	if (v)
	{
		setBRLabels();
		if (buffer->isVisible())
			setBufferLabel();
	}
}

void InfoDock::setBRLabels()
{
	QString abr, vbr, fps;
	if (videoBR > -1)
		vbr = tr("Video bitrate") + ": " + QString::number(videoBR) + "kbps";
	if (videoFPS > -1.0)
		fps = QString::number(videoFPS) + " FPS";
	if (audioBR > -1)
		abr = tr("Audio bitrate") + ": " + QString::number(audioBR) + "kbps";
	bitrate->setText(abr + "\n" + vbr + (!fps.isEmpty() ? (!vbr.isEmpty() ? ", " : "") + fps : ""));
}
void InfoDock::setBufferLabel()
{
	QString txt = tr("Buffer") + ": [" + Functions::sizeString(bytes1) + ", " + Functions::sizeString(bytes2) + "]";
	if (seconds1 >= 0.0 && seconds2 >= 0.0)
		txt += ", [" + QString::number(seconds1, 'f', 1) + " s" + ", " + QString::number(seconds2, 'f', 1) + " s" + "]";
	buffer->setText(txt);
}
