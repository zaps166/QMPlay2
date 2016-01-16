#include <Module.hpp>

class Subtitles : public Module
{
public:
	Subtitles();
private:
	QList< Info > getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QCheckBox;
class QDoubleSpinBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
public:
	ModuleSettingsWidget(Module &);
private:
	void saveSettings();

	QCheckBox *srtEB, *classicEB, *mEB;
	QDoubleSpinBox *maxLenB;
};
