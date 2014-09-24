#include <DirectX.hpp>
#include <DirectDraw.hpp>

DirectX::DirectX() :
	Module( "DirectX" )
{
	moduleImg = QImage( ":/DirectX" );

	init( "DirectDraw", true );
// 	init( "Direct3D", true );
	init( "DirectDraw/OnlyOverlay", true );
}

QList< DirectX::Info > DirectX::getModulesInfo( const bool showDisabled ) const
{
	QList< Info > modulesInfo;
	if ( showDisabled || getBool( "DirectDraw" ) )
		modulesInfo += Info( DirectDrawWriterName, WRITER, QStringList( "video" ) );
	return modulesInfo;
}
void *DirectX::createInstance( const QString &name )
{
	if ( name == DirectDrawWriterName && getBool( "DirectDraw" ) )
		return new DirectDrawWriter( *this );
	return NULL;
}

DirectX::SettingsWidget *DirectX::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_PLUGIN( DirectX )

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	ddrawB = new QCheckBox( tr( "Włączony" ) );
	ddrawB->setChecked( sets().getBool( "DirectDraw" ) );

	ddrawOnlyOverlayB = new QCheckBox( tr( "Użyj DirectDraw tylko w trybie nakładki" ) );
	ddrawOnlyOverlayB->setToolTip( tr( "Jeżeli wyłączysz tę opcję to DirectDraw zostanie użyty jeżeli nakładka nie będzie obsługiwana. Na razie działa to nieprawidłowo!" ) );
	ddrawOnlyOverlayB->setChecked( sets().getBool( "DirectDraw/OnlyOverlay" ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( ddrawB );
	layout->addWidget( ddrawOnlyOverlayB );
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "DirectDraw", ddrawB->isChecked() );
	sets().set( "DirectDraw/OnlyOverlay", ddrawOnlyOverlayB->isChecked() );
}
