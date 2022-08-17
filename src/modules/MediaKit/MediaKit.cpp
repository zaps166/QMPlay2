#include <MediaKit.hpp>
#include <MediaKitWriter.hpp>

MediaKit::MediaKit() :
	Module( "MediaKit" )
{
	m_icon = QIcon( ":/MediaKit" );

	init( "WriterEnabled", true );
	init( "Delay", 0.2 );
}

QList< MediaKit::Info > MediaKit::getModulesInfo( const bool showDisabled ) const
{
	QList< Info > modulesInfo;
	if ( showDisabled || getBool( "WriterEnabled" ) )
		modulesInfo += Info( MediaKitWriterName, WRITER, QStringList( "audio" ) );
	return modulesInfo;
}
void *MediaKit::createInstance( const QString &name )
{
	if ( name == MediaKitWriterName && getBool( "WriterEnabled" ) )
		return new MediaKitWriter( *this );
	return NULL;
}

MediaKit::SettingsWidget *MediaKit::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_MODULE( MediaKit )

/**/

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	enabledB = new QCheckBox( tr( "Enabled" ) );
	enabledB->setChecked( sets().getBool( "WriterEnabled" ) );

	QLabel *delayL = new QLabel( tr( "Delay" ) + ": " );

	delayB = new QDoubleSpinBox;
	delayB->setRange( 0.01, 1.0 );
	delayB->setSingleStep( 0.01 );
	delayB->setSuffix( " " + tr( "sec" ) );
	delayB->setValue( sets().getDouble( "Delay" ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( enabledB, 0, 0, 1, 2 );
	layout->addWidget( delayL, 1, 0, 1, 1 );
	layout->addWidget( delayB, 1, 1, 1, 1 );
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "WriterEnabled", enabledB->isChecked() );
	sets().set( "Delay", delayB->value() );
}
