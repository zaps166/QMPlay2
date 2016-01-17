#include <DockWidget.hpp>

#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050600
#include <QTimer>

void DockWidget::EmptyW::showEvent(QShowEvent *) //This should fix some issues on Qt5 (QTBUG-49445)
{
	QTimer::singleShot(0, this, SLOT(hide()));
}
#endif

QSize DockWidget::EmptyW::sizeHint() const
{
	return QSize(0, 0);
}
