#include <OpenGLES.hpp>
#include <OpenGLESWriter.hpp>

OpenGLES::OpenGLES() :
	Module( "OpenGLES" )
{
	moduleImg = QImage( ":/OpenGLES" );

	init( "Enabled", true );
	init( "VSync", true );
}

QList< OpenGLES::Info > OpenGLES::getModulesInfo( const bool showDisabled ) const
{
	QList< Info > modulesInfo;
	if ( showDisabled || getBool( "Enabled" ) )
		modulesInfo += Info( OpenGLESWriterName, WRITER, QStringList( "video" ) );
	return modulesInfo;
}
void *OpenGLES::createInstance( const QString &name )
{
	if ( name == OpenGLESWriterName && getBool( "Enabled" ) )
		return new OpenGLESWriter( *this );
	return NULL;
}

OpenGLES::SettingsWidget *OpenGLES::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_PLUGIN( OpenGLES )

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	enabledB = new QCheckBox( tr( "Włączony" ) );
	enabledB->setChecked( sets().getBool( "Enabled" ) );

	vsyncB = new QCheckBox( tr( "Synchronizacja pionowa" ) +  " (VSync)" );
	vsyncB->setChecked( sets().getBool( "VSync" ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( enabledB );
	layout->addWidget( vsyncB );
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "Enabled", enabledB->isChecked() );
	sets().set( "VSync", vsyncB->isChecked() );
}
