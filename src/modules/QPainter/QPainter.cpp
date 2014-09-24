#include <QPainter.hpp>
#include <QPainterWriter.hpp>

QPainter_Qt::QPainter_Qt() :
	Module( "QPainter_Qt" )
{
	moduleImg = QImage( ":/Qt" );

	init( "Enabled", true );
}

QList< QPainter_Qt::Info > QPainter_Qt::getModulesInfo( const bool showDisabled ) const
{
	QList< Info > modulesInfo;
	if ( showDisabled || getBool( "Enabled" ) )
		modulesInfo += Info( QPainterWriterName, WRITER, QStringList( "video" ) );
	return modulesInfo;
}
void *QPainter_Qt::createInstance( const QString &name )
{
	if ( name == QPainterWriterName && getBool( "Enabled" ) )
		return new QPainterWriter( *this );
	return NULL;
}

QPainter_Qt::SettingsWidget *QPainter_Qt::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_PLUGIN( QPainter_Qt )

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	enabledB = new QCheckBox( tr( "Włączony" ) );
	enabledB->setChecked( sets().getBool( "Enabled" ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( enabledB );
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "Enabled", enabledB->isChecked() );
}
