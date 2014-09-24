#include <Module.hpp>

class Readers : public Module
{
public:
	Readers();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QCheckBox;
class QSpinBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS( ModuleSettingsWidget )
public:
	ModuleSettingsWidget( Module & );
private:
	void saveSettings();

	QCheckBox *fileReaderEB, *fileWriterEB, *httpReaderEB;
	QSpinBox *tcpTimeoutB;
};
