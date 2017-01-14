/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <InDockW.hpp>

#include <Settings.hpp>

#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QCoreApplication>
#include <QGraphicsScene>
#include <QDockWidget>
#include <QPainter>
#include <QVariant>

#include <math.h>

static QPixmap getBlurred(const QPixmap &input, Qt::TransformationMode &blurredTransformation)
{
	const qreal blurRadius = qBound(10.0, sqrt(input.width() * input.width() + input.height() * input.height()) / 4.0, 300.0);
	blurredTransformation = (blurRadius < 80.0) ? Qt::SmoothTransformation : Qt::FastTransformation;

	QGraphicsBlurEffect *blur = new QGraphicsBlurEffect;
	blur->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
	blur->setBlurRadius(blurRadius);

	QGraphicsPixmapItem *item = new QGraphicsPixmapItem(input);
	item->setGraphicsEffect(blur);

	QGraphicsScene scene;
	scene.addItem(item);

	QPixmap blurred(input.size());
	blurred.fill(Qt::black);

	QPainter p(&blurred);
	scene.render(&p);

	return blurred;
}

/**/

InDockW::InDockW(const QPixmap &qmp2Pixmap, const QColor &grad1, const QColor &grad2, const QColor &qmpTxt) :
	grad1(grad1), grad2(grad2), qmpTxt(qmpTxt),
	qmp2Pixmap(qmp2Pixmap),
	hasWallpaper(false),
	loseHeight(0),
	w(NULL)
{
	connect(&QMPlay2Core, SIGNAL(wallpaperChanged(bool, double)), this, SLOT(wallpaperChanged(bool, double)));
	setAttribute(Qt::WA_OpaquePaintEvent);
	setFocusPolicy(Qt::StrongFocus);
	grabGesture(Qt::PinchGesture);
	setAutoFillBackground(true);
	setMouseTracking(true);
}

QWidget *InDockW::getWidget()
{
	return w;
}
void InDockW::setLoseHeight(int lh)
{
	loseHeight = lh;
}
void InDockW::setCustomPixmap(const QPixmap &pix)
{
	customPixmap = pix;
	if (customPixmap.isNull() || !QMPlay2Core.getSettings().getBool("BlurCovers"))
		customPixmapBlurred = QPixmap();
	else
		customPixmapBlurred = getBlurred(pix, blurredTransformation);
	emit hasCoverImage(!customPixmap.isNull());
	update();
}

void InDockW::wallpaperChanged(bool wallpaper, double alpha)
{
	QColor c = Qt::black;
	hasWallpaper = wallpaper;
	setAttribute(Qt::WA_OpaquePaintEvent, !wallpaper);
	if (wallpaper)
		c.setAlphaF(alpha);
	setPalette(c);
}
void InDockW::setWidget(QWidget *_w)
{
	if (w == _w)
		return;
	if (w)
	{
		disconnect(w, SIGNAL(destroyed()), this, SLOT(nullWidget()));
		w->setParent(NULL);
	}
	if ((w = _w))
	{
#if QT_VERSION < 0x050000
		//Workaround for BUG in Qt4
		QWidget *mainWidget = window();
		if (mainWidget && mainWidget->isMinimized() && w->internalWinId() && !internalWinId())
		{
			mainWidget->hide();
			mainWidget->show();
		}
#endif

		w->setMinimumSize(2, 2);
		w->setParent(this);
		resizeEvent(NULL);
		w->show();
		connect(w, SIGNAL(destroyed()), this, SLOT(nullWidget()));
	}
}

void InDockW::nullWidget()
{
	w = NULL;
}

void InDockW::resizeEvent(QResizeEvent *)
{
	if (w)
	{
		int X = 0;
		int Y = 0;
		int W = width();
		int H = height() + loseHeight;

		const int mappedY = mapToParent(QPoint()).y();
		if (mappedY < 0)
		{
			H += mappedY;
			Y -= mappedY;
		}

		const Qt::CheckState preventFullscreen = (Qt::CheckState)w->property("preventFullscreen").value<int>();
		if ((preventFullscreen == Qt::Checked || (loseHeight && preventFullscreen == Qt::PartiallyChecked)) && window()->property("fullScreen").toBool())
		{
			X -= 1;
			Y -= 1;
			W += 2;
			H += 2;
		}

		if (w->geometry() != QRect(X, Y, W, H))
		{
			w->setGeometry(X, Y, W, H);
			emit resized(W, H);
		}
	}
}
void InDockW::paintEvent(QPaintEvent *)
{
	if (!w)
	{
		QPainter p(this);

		bool isFloating = false;
		if (QDockWidget *dW = qobject_cast<QDockWidget *>(parentWidget()))
			isFloating = dW->isFloating();

		const int fullHeight = height() + loseHeight;
		const bool drawBackground = (isFloating || !hasWallpaper);
		const bool drawBlurredImage = drawBackground && !customPixmapBlurred.isNull();

		if (drawBackground && !drawBlurredImage)
		{
			if (grad1 == grad2)
				p.fillRect(rect(), grad1);
			else
			{
				QLinearGradient lgrad(width() / 2, 0, width() / 2, fullHeight);
				lgrad.setColorAt(0.0, grad1);
				lgrad.setColorAt(0.5, grad2);
				lgrad.setColorAt(1.0, grad1);
				p.fillRect(rect(), lgrad);
			}
		}

		if (customPixmap.isNull())
		{
			p.drawPixmap(width() / 2 - qmp2Pixmap.width() / 2, fullHeight / 2 - qmp2Pixmap.height() / 2, qmp2Pixmap);

			QFont font = p.font();
			font.setPointSize(22);
			font.setItalic(true);
			p.setFont(font);
			p.setPen(qmpTxt);
			p.drawText(0, fullHeight / 2 + qmp2Pixmap.height() / 2, width(), 100, Qt::AlignHCenter | Qt::AlignTop, "QMPlay2");
		}
		else
		{
			QPixmap pixmapToDraw;
			float multiplier;

			if (!drawBlurredImage)
				multiplier = 1.0f;
			else
			{
				const QPixmap blurred = customPixmapBlurred.scaled(width(), fullHeight, Qt::KeepAspectRatioByExpanding, blurredTransformation);
				p.drawPixmap(width() / 2 - blurred.width() / 2, fullHeight / 2 - blurred.height() / 2, blurred);
				multiplier = 0.8f;
			}

			if (customPixmap.width() > width() * multiplier || customPixmap.height() > fullHeight * multiplier)
				pixmapToDraw = customPixmap.scaled(width() * multiplier, fullHeight * multiplier, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			else
				pixmapToDraw = customPixmap;

			p.drawPixmap(width() / 2 - pixmapToDraw.width() / 2, fullHeight / 2 - pixmapToDraw.height() / 2, pixmapToDraw);
		}
	}
}

bool InDockW::event(QEvent *e)
{
	/* Pass gesture and touch event to the parent */
	switch (e->type())
	{
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		case QEvent::TouchEnd:
		case QEvent::Gesture:
			return QCoreApplication::sendEvent(parent(), e);
		default:
			return QWidget::event(e);
	}
}
