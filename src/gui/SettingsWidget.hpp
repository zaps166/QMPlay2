#ifndef SETTINGSWIDHET_HPP
#define SETTINGSWIDHET_HPP

#include <QWidget>

class QListWidgetItem;
class QGridLayout;
class QPushButton;
class QTabWidget;
class Page1;
class Page2;
class Page3;
class Page4;
class Page5;
class Page6;

class SettingsWidget : public QWidget
{
	Q_OBJECT
public:
	static void InitSettings();
	static void SetAudioChannelsMenu();
	static void SetAudioChannels( int chn );

	SettingsWidget( int page = 0, const QString &module = QString() );

	void setAudioChannels();
private:
	static void applyProxy();

	void restartApp();

	void showEvent( QShowEvent * );
	void closeEvent( QCloseEvent * );

	Page1 *page1;
	Page2 *page2;
	Page3 *page3;
	Page4 *page4;
	Page5 *page5;
	Page6 *page6;

	QGridLayout *layout;
	QTabWidget *tabW;
	QPushButton *applyB, *closeB;
	QString lastM[ 3 ];

	bool wasShow;
	int moduleIndex;
private slots:
	void chStyle();
	void apply();
	void chModule( QListWidgetItem * );
	void tabCh( int );
	void openModuleSettings( QListWidgetItem * );
	void moveModule();
	void chooseScreenshotDir();
	void page2EnableOrDisable();
	void setAppearance();
	void clearCoversCache();
	void resetSettings();
signals:
	void settingsChanged( int, bool );
	void setWheelStep( int );
	void setVolMax( int );
};

#endif // SETTINGSWIDHET_HPP
