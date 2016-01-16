#include <Module.hpp>

class ALSA : public Module
{
public:
	ALSA();
private:
	QList< Info > getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QDoubleSpinBox;
class QCheckBox;
class QComboBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
	ModuleSettingsWidget(Module &);
private:
	void saveSettings();

	QCheckBox *enabledB, *autoMultichnB;
	QDoubleSpinBox *delayB;
	QComboBox *devicesB;
};
