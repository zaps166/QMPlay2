#ifndef MENUBAR_HPP
#define MENUBAR_HPP

#include <QCoreApplication>
#include <QMenuBar>

class VideoEqualizer;

class MenuBar : public QMenuBar
{
	Q_OBJECT
public:
	MenuBar();

	class Window : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Window)
	public:
		Window(MenuBar *parent);
		QAction *toggleVisibility, *toggleFullScreen, *toggleCompactView, *close;
	};

	class Widgets : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Widgets)
		friend class MenuBar;
	public:
		Widgets(MenuBar *parent);
	private:
		void menuShow();
	};

	class Playlist : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Playlist)
	public:
		Playlist(MenuBar *parent);
		class Add : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS(Add)
		public:
			Add(QMenu *parent);
			QAction *address, *file, *dir;
		};
		class Sort : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS(Sort)
		public:
			Sort(QMenu *parent);
			QAction *timeSort1, *timeSort2, *titleSort1, *titleSort2;
		};
		Add *add;
		QMenu *extensions;
		Sort *sort;
		QAction *stopLoading, *sync, *loadPlist, *savePlist, *saveGroup, *delEntries, *delNonGroupEntries, *clear, *copy, *paste, *newGroup, *renameGroup, *find, *collapseAll, *expandAll, *goToPlayback, *queue, *entryProperties;
	};

	class Player : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Player)
	public:
		Player(MenuBar *parent);
		class Repeat : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS(Repeat)
		public:
			Repeat(QMenu *parent);
			QActionGroup *choice;
			QAction *normal, *repeatEntry, *repeatGroup, *repeatList, *random, *randomGroup;
		};
		class AspectRatio : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS(AspectRatio)
		public:
			AspectRatio(QMenu *parent);
			QActionGroup *choice;
			QAction *_auto, *_1x1, *_4x3, *_5x4, *_16x9, *_3x2, *_21x9, *sizeDep, *off;
		};
		void seekActionsEnable(bool);
		Repeat *repeat;
		AspectRatio *aRatio;
		QAction *togglePlay, *stop, *next, *prev, *nextFrame, *seekF, *seekB, *lSeekB, *lSeekF, *speedUp, *slowDown, *setSpeed, *switchARatio, *zoomIn, *zoomOut, *reset, *volUp, *volDown, *toggleMute, *suspend;
	};

	class Playback : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Playback)
	public:
		Playback(MenuBar *parent);

		class VideoFilters : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS(VideoFilters)
		public:
			VideoFilters(QMenu *parent);
			VideoEqualizer *videoEqualizer;
			QAction *more, *hFlip, *vFlip;
		};
		class AudioChannels : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS(AudioChannels)
		public:
			AudioChannels(QMenu *parent);
			QAction *_auto, *_1, *_2, *_4, *_6, *_8, *other;
			QActionGroup *choice;
		};
		AudioChannels *audioChannels;
		VideoFilters *videoFilters;
		QAction *toggleAudio, *toggleVideo, *videoSync, *slowDownVideo, *speedUpVideo, *toggleSubtitles, *subsFromFile, *subtitlesSync, *slowDownSubtitles, *speedUpSubtitles, *biggerSubtitles, *smallerSubtitles, *playbackSettings, *screenShot;
	};

	class Options : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Options)
	public:
		Options(MenuBar *parent);
		QAction *settings, *modulesSettings, *trayVisible;
	};

	class Help : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Help)
	public:
		Help(MenuBar *parent);
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
	Playback *playback;
	Options *options;
	Help *help;
private slots:
	void widgetsMenuShow();
};

#endif //MENUBAR_HPP
