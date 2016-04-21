#include <Module.hpp>

class FileAssociation : public Module
{
	Q_OBJECT
public:
	FileAssociation();
private:
	QList<Info> getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();

	bool reallyFirsttime;
private slots:
	void firsttime();
};

/**/

class QListWidget;
class QPushButton;
class QGroupBox;
class QCheckBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_OBJECT
	friend class FileAssociation;
public:
	ModuleSettingsWidget(Module &);
private:
	void saveSettings();
	void addExtension(const QString &, const bool, const bool isPlaylist = false);
private slots:
	void selectAll();
	void checkSelected();
private:
	bool ignoreCheck;

	QGroupBox *associateB;
	QPushButton *selectAllB;
	QListWidget *extensionLW;
	QCheckBox *addDirB, *addDrvB, *audioCDB;
};
