#ifndef MAINWIDGET_HPP
#define MAINWIDGET_HPP

#include <QMainWindow>
#include <QSystemTrayIcon>

#include <PlayClass.hpp>
#ifdef UPDATER
	#include <Updater.hpp>
#endif

class QFrame;
class QLabel;
class Slider;
class MenuBar;
class InfoDock;
class VideoDock;
class QToolButton;
class AboutWidget;
class PlaylistDock;
class SettingsWidget;
class QMPlay2Extensions;

class MainWidget : public QMainWindow
{
	friend class QMPlay2GUIClass;
	Q_OBJECT
public:
	MainWidget( QPair< QStringList, QStringList > & );
	~MainWidget();
private slots:
	void focusChanged( QWidget *, QWidget * );

	void processParam( const QString &, const QString & );

	void audioChannelsChanged();

	void updateWindowTitle( const QString &t = QString() );
	void videoStarted();

	void togglePlay();
	void seek( int );
	void chStream( const QString & );
	void playStateChanged( bool );

	void volUpDown();
	void actionSeek();
	void switchARatio();
	void resetARatio();
	void resetFlip();

	void visualizationFullScreen();
	void hideAllExtensions();
	void toggleVisibility();
	void createMenuBar();
	void trayIconClicked( QSystemTrayIcon::ActivationReason );
	void toggleCompactView();
	void toggleFullScreen();
	void showMessage( const QString &, const QString &, int, int );
	void statusBarMessage( const QString & );

	void openUrl();
	void openFiles();
	void openDir();
	void loadPlist();
	void savePlist();
	void saveGroup();
	void showSettings( const QString & );
	void showSettings();

	void itemDropped( const QString &, bool );
	void browseSubsFile();

	void updatePos( int );
	void mousePositionOnSlider( int );

	void newConnection();
	void readSocket();

	void about();

#ifdef Q_OS_WIN
	void console( bool );
#endif
	void lockWidgets( bool );

	void hideDocksSlot();
private:
	void savePlistHelper( const QString &, const QString &, bool );

	QMenu *createPopupMenu();

	void showToolBar( bool );

	void hideDocks();

	void mouseMoveEvent( QMouseEvent * );
	void leaveEvent( QEvent * );
	void closeEvent( QCloseEvent * );
	void moveEvent( QMoveEvent * );
	void showEvent( QShowEvent * );
	void hideEvent( QHideEvent * );
#ifdef Q_OS_WIN
	bool winEvent( MSG *, long * );
#endif

	MenuBar *menuBar;
	QToolBar *mainTB;
	QStatusBar *statusBar;

	QFrame *vLine;
	QLabel *timeL, *stateL;

	VideoDock *videoDock;
	InfoDock *infoDock;
	PlaylistDock *playlistDock;

	Slider *seekS, *volS;

	PlayClass playC;

	QSystemTrayIcon *tray;
	QByteArray dockWidgetState, fullScreenDockWidgetState;
	QRect savedGeo;
	QList< QMPlay2Extensions * > visibleQMPlay2Extensions;
	SettingsWidget *settingsW;
	AboutWidget *aboutW;
	bool isCompactView, wasShow, fullScreen;

#ifdef UPDATER
	Updater updater;
#endif
};

#endif //MAINWIDGET_HPP
