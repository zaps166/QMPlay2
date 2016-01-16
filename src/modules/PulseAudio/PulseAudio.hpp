#include <Module.hpp>

class PulseAudio : public Module
{
public:
	PulseAudio();
private:
	QList< Info > getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QDoubleSpinBox;
class QCheckBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
	ModuleSettingsWidget(Module &);
private:
	void saveSettings();

	QCheckBox *enabledB;
	QDoubleSpinBox *delayB;
};
