#ifndef MENUBAR_HPP
#define MENUBAR_HPP

#include <QApplication>
#include <QMenuBar>

class QWidgetAction;
class VideoEqualizer;

class MenuBar : public QMenuBar
{
	Q_OBJECT
public:
	MenuBar();

	class Window : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS( Window )
	public:
		Window( MenuBar *parent, const QString &name = tr( "&Okno" ) );
#ifdef Q_OS_WIN
		QAction *console;
#endif
		QAction *toggleVisibility, *toggleFullScreen, *toggleCompactView, *close;
	};

	class Widgets : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS( Widgets )
		friend class MenuBar;
	public:
		Widgets( MenuBar *parent, const QString &name = tr( "&Widgety" ) );
	private:
		void menuShow();
	};

	class Playlist : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS( Playlist )
	public:
		Playlist( MenuBar *parent, const QString &name = tr( "&Lista odtwarzania" ) );
		class Add : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS( Add )
		public:
			Add( QMenu *, const QString &name = tr( "&Dodaj" ) );
			QAction *address, *file, *dir;
		};
		class Sort : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS( Sort )
		public:
			Sort( QMenu *, const QString &name = tr( "S&ortuj" ) );
			QAction *timeSort1, *timeSort2, *titleSort1, *titleSort2;
		};
		Add *add;
		QMenu *extensions;
		Sort *sort;
		QAction *stopLoading, *sync, *loadPlist, *savePlist, *saveGroup, *delEntries, *delNonGroupEntries, *clear, *copy, *paste, *newGroup, *renameGroup, *find, *collapseAll, *expandAll, *goToPlaying, *queue, *entryProperties;
	};

	class Player : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS( Player )
	public:
		Player( MenuBar *parent, const QString &name = tr( "O&dtwarzacz" ) );
		class Repeat : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS( Repeat )
		public:
			Repeat( QMenu *, const QString &name = tr( "Zapętlenie &odtwarzania" ) );
			QActionGroup *choice;
			QAction *normal, *repeatEntry, *repeatGroup, *repeatList, *random, *randomGroup;
		};
		class AspectRatio : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS( AspectRatio )
		public:
			AspectRatio( QMenu *, const QString &name = tr( "W&spółczynnik proporcji" ) );
			QActionGroup *choice;
			QAction *_auto, *_1x1, *_4x3, *_5x4, *_16x9, *_3x2, *_21x9, *sizeDep, *off;
		};
		void seekActionsEnable( bool );
		Repeat *repeat;
		AspectRatio *aRatio;
		QAction *togglePlay, *stop, *next, *prev, *nextFrame, *seekF, *seekB, *lSeekB, *lSeekF, *speedUp, *slowDown, *setSpeed, *switchARatio, *zoomIn, *zoomOut, *reset, *volUp, *volDown, *toggleMute;
	};

	class Playing : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS( Playing )
	public:
		Playing( MenuBar *parent, const QString &name = tr( "Od&twarzanie" ) );

		class VideoFilters : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS( VideoFilters )
		public:
			VideoFilters( QMenu *, const QString &name = tr( "Filtry wid&eo" ) );
			VideoEqualizer *videoEqualizer;
			QAction *more, *hFlip, *vFlip;
		};
		class AudioChannels : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS( AudioChannels )
		public:
			AudioChannels( QMenu *, const QString &name = tr( "&Kanały" ) );
			QAction *_auto, *_1, *_2, *_4, *_6, *_8, *other;
			QActionGroup *choice;
		};
		AudioChannels *audioChannels;
		VideoFilters *videoFilters;
		QAction *toggleAudio, *toggleVideo, *videoSync, *slowDownVideo, *speedUpVideo, *toggleSubtitles, *subsFromFile, *subtitlesSync, *slowDownSubtitles, *speedUpSubtitles, *biggerSubtitles, *smallerSubtitles, *playingSettings, *screenShot;
	};

	class Options : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS( Options )
	public:
		Options( MenuBar *parent, const QString &name = tr( "Op&cje" ) );
		QAction *settings, *modulesSettings, *trayVisible;
	};

	class Help : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS( Help )
	public:
		Help( MenuBar *parent, const QString &name = tr( "Po&moc" ) );
		QAction *about,
#ifdef UPDATER
		*updates,
#endif
		*aboutQt;
	};

	Window *window;
	Widgets *widgets;
	Playlist *playlist;
	Player *player;
	Playing *playing;
	Options *options;
	Help *help;
private slots:
	void widgetsMenuShow();
};

#endif //MENUBAR_HPP
