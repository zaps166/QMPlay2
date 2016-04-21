#include <Module.hpp>

class OpenSLES : public Module
{
public:
	OpenSLES();
private:
	QList<Info> getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();
};

/**/

class QCheckBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
public:
	ModuleSettingsWidget(Module &);
private:
	void saveSettings();

	QCheckBox *openslesB;
};
