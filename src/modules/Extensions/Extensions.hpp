#include <Module.hpp>

class Extensions : public Module
{
public:
	Extensions();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	SettingsWidget *getSettingsWidget();

	QImage downloader, youtube, radio, lastfm;
};

/**/

class QToolButton;
class QListWidget;
class QGroupBox;
class QCheckBox;
class LineEdit;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_OBJECT
public:
	ModuleSettingsWidget( Module & );
private slots:
	void browseYoutubedl();
	void loginPasswordEnable( bool checked );
	void passwordEdited();
private:
	void saveSettings();

#ifdef USE_MPRIS2
	QGroupBox *MPRIS2B;
	QCheckBox *exportCoversB;
#endif

	QCheckBox *additionalInfoB;
	LineEdit *youtubedlE;
	QToolButton *youtubedlBrowseB;
	QListWidget *itagLW;

	QCheckBox *downloadCoversB, *allowBigCovers, *updateNowPlayingAndScrobbleB;
	LineEdit *loginE, *passwordE;
};
