#include <Readers.hpp>
#include <FileReader.hpp>
#include <HttpReader.hpp>

Readers::Readers() :
	Module( "Readers" )
{
	init( "File/ReaderEnabled", true );
	init( "Http/ReaderEnabled", true );
	init( "Http/TCPTimeout", 15 );
}

QList< Readers::Info > Readers::getModulesInfo( const bool showDisabled ) const
{
	QList< Info > modulesInfo;
	if ( showDisabled || getBool( "File/ReaderEnabled" ) )
		modulesInfo += Info( FileReaderName, READER, QStringList( "file" ) );
	if ( showDisabled || getBool( "Http/ReaderEnabled" ) )
		modulesInfo += Info( HttpReaderName, READER, QStringList() << "http" << "https" );
	return modulesInfo;
}
void *Readers::createInstance( const QString &name )
{
	if ( name == FileReaderName && getBool( "File/ReaderEnabled" ) )
		return new FileReader;
	else if ( name == HttpReaderName && getBool( "Http/ReaderEnabled" ) )
		return new HttpReader( *this );
	return NULL;
}

Readers::SettingsWidget *Readers::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_PLUGIN( Readers )

/**/

#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	fileReaderEB = new QCheckBox( tr( "Odczyt plików" ) );
	fileReaderEB->setChecked( sets().getBool( "File/ReaderEnabled" ) );

	httpReaderEB = new QCheckBox( "HttpReader " + tr( "włączony" ) );
	httpReaderEB->setChecked( sets().getBool( "Http/ReaderEnabled" ) );

	QLabel *timeoutL = new QLabel( tr( "Czas oczekiwania dla TCP" ) + ": " );

	tcpTimeoutB = new QSpinBox;
	tcpTimeoutB->setRange( 2, 25 );
	tcpTimeoutB->setSuffix( " " + tr( "sek" ) );
	tcpTimeoutB->setValue( sets().getInt( "Http/TCPTimeout" ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( fileReaderEB, 0, 0, 1, 2 );
	layout->addWidget( httpReaderEB, 1, 0, 1, 2 );
	layout->addWidget( timeoutL, 2, 0, 1, 1 );
	layout->addWidget( tcpTimeoutB, 2, 1, 1, 1 );
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "File/ReaderEnabled", fileReaderEB->isChecked() );
	sets().set( "Http/ReaderEnabled", httpReaderEB->isChecked() );
	sets().set( "Http/TCPTimeout", tcpTimeoutB->value() );
}
