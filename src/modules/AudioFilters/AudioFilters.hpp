#include <Module.hpp>

class AudioFilters : public Module
{
public:
	AudioFilters();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	SettingsWidget *getSettingsWidget();
};

/**/

class QGroupBox;
class QCheckBox;
class QComboBox;
class QSpinBox;
class Slider;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_OBJECT
public:
	ModuleSettingsWidget( Module & );
private slots:
	void voiceRemovalToggle();
	void phaseReverse();
	void echo();
private:
	void saveSettings();

	QCheckBox *voiceRemovalEB, *phaseReverseEB, *phaseReverseRightB;

	QGroupBox *echoB;
	Slider *echoDelayB, *echoVolumeB, *echoFeedbackB;
	QCheckBox *echoSurroundB;

	QComboBox *eqQualityB;
	QSpinBox *eqSlidersB, *eqMinFreqB, *eqMaxFreqB;
};
