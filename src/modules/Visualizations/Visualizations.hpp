#include <Module.hpp>

class Visualizations : public Module
{
public:
	Visualizations();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QSpinBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_DECLARE_TR_FUNCTIONS( ModuleSettingsWidget )
public:
	ModuleSettingsWidget( Module & );
private:
	void saveSettings();

	QSpinBox *refTimeB, *sndLenB, *fftSizeB, *fftScaleB;
};
