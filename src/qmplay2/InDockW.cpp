#include <InDockW.hpp>

#include <QMPlay2Core.hpp>

#include <QCoreApplication>
#include <QPainter>
#include <QVariant>

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

void InDockW::setCustomPixmap(const QPixmap &pix)
{
	customPixmap = pix;
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
		int mappedY = mapToParent(QPoint()).y();
		int Y = 0;
		int W = width();
		int H = height() + loseHeight;
		if (mappedY < 0)
		{
			H += mappedY;
			Y -= mappedY;
		}
		if (loseHeight && w->property("PreventFullscreen").toBool())
		{
			Y -= 1;
			H += 2;
		}
		if (w->geometry() != QRect(0, Y, W, H))
		{
			w->setGeometry(0, Y, W, H);
			emit resized(W, H);
		}
	}
}
void InDockW::paintEvent(QPaintEvent *)
{
	if (!w)
	{
		QPainter p(this);

		int fullHeight = height() + loseHeight;

		if (!hasWallpaper)
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
			if (customPixmap.width() > width() || customPixmap.height() > fullHeight)
				pixmapToDraw = customPixmap.scaled(width(), fullHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			else
				pixmapToDraw = customPixmap;
			p.drawPixmap(width() / 2 - pixmapToDraw.width() / 2, fullHeight / 2 - pixmapToDraw.height() / 2, pixmapToDraw);
		}
	}
}

bool InDockW::event(QEvent *e)
{
	/* Pass gesture event to the parent */
	if (e->type() == QEvent::Gesture)
		return qApp->notify(parent(), e);
	return QWidget::event(e);
}
