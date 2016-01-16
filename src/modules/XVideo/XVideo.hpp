#include <Module.hpp>

class XVideo : public Module
{
public:
	XVideo();
private:
	QList< Info > getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QCheckBox;
class QComboBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
	ModuleSettingsWidget(Module &);
private:
	void saveSettings();

	QCheckBox *enabledB, *useSHMB;
	QComboBox *adaptorsB;
};
