#include <Module.hpp>

#include <QCoreApplication>

class CDIODestroyTimer;

class AudioCD : public Module
{
	Q_OBJECT
public:
	AudioCD();
	~AudioCD();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	QList< QAction * > getAddActions();

	SettingsWidget *getSettingsWidget();

	CDIODestroyTimer *cdioDestroyTimer;
	QString AudioCDPlaylist;
private slots:
	void add();
	void browseCDImage();
};

/**/

class QGridLayout;
class QGroupBox;
class QCheckBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS( ModuleSettingsWidget )
public:
	ModuleSettingsWidget( Module & );
private:
	void saveSettings();

	QGroupBox *audioCDB;
	QCheckBox *useCDDB, *useCDTEXT;
};
