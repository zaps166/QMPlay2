#include <VisWidget.hpp>
#include <QMPlay2Core.hpp>
#include <DockWidget.hpp>
#include <Functions.hpp>

#include <QMouseEvent>
#include <QMenu>

#include <math.h>

void VisWidget::setValue(qreal &out, qreal in, qreal tDiffScaled)
{
	if (in < out)
		out -= sqrt(out) * tDiffScaled;
	else
		out = in;
}
void VisWidget::setValue(QPair<qreal, double> &out, qreal in, qreal tDiffScaled)
{
	if (in < out.first)
		out.first -= (Functions::gettime() - out.second) * tDiffScaled;
	else
	{
		out.first = in;
		out.second = Functions::gettime();
	}
}

VisWidget::VisWidget() :
	stopped(true),
	dw(new DockWidget)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setFocusPolicy(Qt::StrongFocus);
	setAutoFillBackground(true);
	setMouseTracking(true);
	setPalette(Qt::black);

	connect(&tim, SIGNAL(timeout()), this, SLOT(update()));
	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
#if QT_VERSION >= 0x050000
	connect(&QMPlay2Core, SIGNAL(mainWidgetNotMinimized(bool)), this, SLOT(visibilityChanged(bool)));
#endif
	connect(&QMPlay2Core, SIGNAL(wallpaperChanged(bool, double)), this, SLOT(wallpaperChanged(bool, double)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
}

bool VisWidget::regionIsVisible() const
{
	return dw->visibleRegion() != QRegion() || visibleRegion() != QRegion();
}

void VisWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (parent() == dw && (e->buttons() & Qt::LeftButton))
		emit doubleClicked();
	else
		QWidget::mouseDoubleClickEvent(e);
}
void VisWidget::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::ParentChange && !parent())
		dw->setWidget(this);
	QWidget::changeEvent(event);
}

void VisWidget::wallpaperChanged(bool hasWallpaper, double alpha)
{
	QColor c = Qt::black;
	if (hasWallpaper)
		c.setAlphaF(alpha);
	setPalette(c);
}
void VisWidget::contextMenu(const QPoint &point)
{
	QMenu *menu = new QMenu(this);
	connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));
	connect(menu->addAction(tr("Settings")), SIGNAL(triggered()), this, SLOT(showSettings()));
	menu->popup(mapToGlobal(point));
}
void VisWidget::visibilityChanged(bool v)
{
	if (!v && parent() == dw)
		stop();
	else if (!stopped)
	{
#if QT_VERSION >= 0x050000
		//Don't start visualization on Qt5 when it is invisible and main window is not minimized!
		const bool fromMainWindow = &QMPlay2Core == sender();
#else
		const bool fromMainWindow = false;
#endif
		start(v && (!fromMainWindow || regionIsVisible()), fromMainWindow);
	}
}
void VisWidget::showSettings()
{
	emit QMPlay2Core.showSettings("Visualizations");
}
