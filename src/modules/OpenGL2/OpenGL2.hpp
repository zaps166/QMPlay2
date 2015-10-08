#include <Module.hpp>

class OpenGL2 : public Module
{
public:
	OpenGL2();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QCheckBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS( ModuleSettingsWidget )
public:
	ModuleSettingsWidget( Module & );
private:
	void saveSettings();

	QCheckBox *enabledB;
#ifdef VSYNC_SETTINGS
	QCheckBox *vsyncB;
#endif
};
