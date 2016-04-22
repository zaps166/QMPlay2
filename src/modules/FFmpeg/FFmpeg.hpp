#include <Module.hpp>

class FFmpeg : public Module
{
public:
	FFmpeg();
	~FFmpeg();
private:
	QList<Info> getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();

	/**/

	QMutex mutex;
};

/**/

#include <QCoreApplication>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QSpinBox;
class Slider;

class ModuleSettingsWidget : public Module::SettingsWidget
{
#ifdef QMPlay2_VDPAU
	Q_OBJECT
#else
	Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
#endif
public:
	ModuleSettingsWidget(Module &);
#ifdef QMPlay2_VDPAU
private slots:
	void setVDPAU();
	void checkEnables();
#endif
private:
	void saveSettings();

	QGroupBox *hurryUpB;
	QCheckBox *demuxerEB, *skipFramesB, *forceSkipFramesB;
	QGroupBox *decoderB;
#ifdef QMPlay2_VDPAU
	QGroupBox *decoderVDPAUB;
	QComboBox *vdpauDeintMethodB, *vdpauHQScalingB;
	QCheckBox *noisereductionVDPAUB, *sharpnessVDPAUB;
	Slider *noisereductionLvlVDPAUS, *sharpnessLvlVDPAUS;
	QCheckBox *decoderVDPAU_NWB;
#endif
#ifdef QMPlay2_VAAPI
	QCheckBox *allowVDPAUinVAAPIB;
	QGroupBox *decoderVAAPIEB;
	QComboBox *vaapiDeintMethodB;
#endif
	QSpinBox *threadsB;
	QComboBox *lowresB, *thrTypeB;
};
