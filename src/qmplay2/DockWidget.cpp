#include <DockWidget.hpp>

#include <QTimer>

void DockWidget::EmptyW::showEvent( QShowEvent * ) //This should fix some issues on Qt5
{
	QTimer::singleShot( 0, this, SLOT( hide() ) );
}

QSize DockWidget::EmptyW::sizeHint() const
{
	return QSize( 0, 0 );
}
