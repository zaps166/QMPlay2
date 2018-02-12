/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <RepeatMode.hpp>

#include <QCoreApplication>
#include <QMenuBar>

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

		QAction *hideMenuAct, *lockWidgetsAct;
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
		QAction *stopLoading, *sync, *quickSync, *loadPlist, *savePlist, *saveGroup, *lock, *alwaysSync, *delEntries, *delNonGroupEntries, *clear, *copy, *paste, *newGroup, *renameGroup, *find, *collapseAll, *expandAll, *goToPlayback, *queue, *skip, *stopAfter, *entryProperties;
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
			QAction *repeatActions[RepeatModeCount];
		};
		class AspectRatio : public QMenu
		{
			Q_DECLARE_TR_FUNCTIONS(AspectRatio)
		public:
			AspectRatio(QMenu *parent);
			QActionGroup *choice;
			QAction *_auto, *_1x1, *_4x3, *_5x4, *_16x9, *_3x2, *_21x9, *sizeDep, *off;
		};

		void seekActionsEnable(bool e);
		void playActionEnable(bool e);

		Repeat *repeat;
		AspectRatio *aRatio;
		QAction *togglePlay, *stop, *next, *prev, *prevFrame, *nextFrame, *abRepeat, *seekF, *seekB, *lSeekB, *lSeekF, *speedUp, *slowDown, *setSpeed, *switchARatio, *zoomIn, *zoomOut, *reset, *volUp, *volDown, *toggleMute, *detach, *suspend;
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
			QMenu *videoAdjustmentMenu;
			QAction *spherical, *hFlip, *vFlip, *rotate90, *more;
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
		QAction *toggleAudio, *toggleVideo, *videoSync, *slowDownVideo, *speedUpVideo, *toggleSubtitles, *subsFromFile, *subtitlesSync, *slowDownSubtitles, *speedUpSubtitles, *biggerSubtitles, *smallerSubtitles, *screenShot;
	};

	class Options : public QMenu
	{
		Q_DECLARE_TR_FUNCTIONS(Options)
	public:
		Options(MenuBar *parent);
		QAction *settings, *playbackSettings, *modulesSettings, *trayVisible;
		QMenu *removeProfileMenu;
		QActionGroup *profilesGroup;
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

	void setKeyShortcuts();

	Window *window;
	Widgets *widgets;
	Playlist *playlist;
	Player *player;
	Playback *playback;
	Options *options;
	Help *help;
public slots:
	void changeProfile();
	void addProfile();
	void copyProfile();
	void removeProfileMenuRequest(const QPoint &p);
	void removeProfile();
private slots:
	void widgetsMenuShow();
};
