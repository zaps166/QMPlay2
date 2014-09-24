#include <Module.hpp>

class PortAudio : public Module
{
public:
	PortAudio();
	~PortAudio();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	SettingsWidget *getSettingsWidget();
};

/**/

class QDoubleSpinBox;
class QCheckBox;
class QComboBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_OBJECT
public:
	ModuleSettingsWidget( Module & );
private slots:
	void defaultDevs();
private:
	void saveSettings();

	QCheckBox *enabledB;
	QComboBox *devicesB;
	QDoubleSpinBox *delayB;
};
