#include <OpenGL.hpp>
#include <OpenGLWriter.hpp>

OpenGL::OpenGL() :
	Module( "OpenGL" )
{
	moduleImg = QImage( ":/OpenGL" );

	init( "Enabled", true );
	init( "VSync", true );
	init( "Use_shaders", true );
}

QList< OpenGL::Info > OpenGL::getModulesInfo( const bool showDisabled ) const
{
	QList< Info > modulesInfo;
	if ( showDisabled || getBool( "Enabled" ) )
		modulesInfo += Info( OpenGLWriterName, WRITER, QStringList( "video" ) );
	return modulesInfo;
}
void *OpenGL::createInstance( const QString &name )
{
	if ( name == OpenGLWriterName && getBool( "Enabled" ) )
		return new OpenGLWriter( *this );
	return NULL;
}

OpenGL::SettingsWidget *OpenGL::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_PLUGIN( OpenGL )

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

	shadersB = new QCheckBox( tr( "Używaj shaderów do konwersji YUV->RGB" ) );
	shadersB->setChecked( sets().getBool( "Use_shaders" ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( enabledB );
	layout->addWidget( vsyncB );
	layout->addWidget( shadersB );
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "Enabled", enabledB->isChecked() );
	sets().set( "VSync", vsyncB->isChecked() );
	sets().set( "Use_shaders", shadersB->isChecked() );
}
