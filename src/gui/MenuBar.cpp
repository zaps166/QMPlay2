#include <MenuBar.hpp>

#include <VideoEqualizer.hpp>
#include <DockWidget.hpp>
#include <Main.hpp>

#include <QWidgetAction>
#include <QMainWindow>
#include <QDebug>

static QAction *newAction( const QString &txt, QMenu *parent, const QKeySequence &keySequence, QAction *&act, bool autoRepeat, const QIcon &icon, bool checkable )
{
	act = new QAction( txt, parent );
	act->setAutoRepeat( autoRepeat );
	act->setShortcut( keySequence );
	act->setCheckable( checkable );
	parent->addAction( act );
	act->setIcon( icon );
	return act;
}

/**/

MenuBar::MenuBar()
{
	addMenu( window = new Window( this ) );
	addMenu( widgets = new Widgets( this ) );
	addMenu( playlist = new Playlist( this ) );
	addMenu( player = new Player( this ) );
	addMenu( playing = new Playing( this ) );
	addMenu( options = new Options( this ) );
	addMenu( help = new Help( this ) );
	connect( widgets, SIGNAL( aboutToShow() ), this, SLOT( widgetsMenuShow() ) );
}

MenuBar::Window::Window( MenuBar *parent, const QString &name ) : QMenu( name, parent )
{
#ifdef Q_OS_WIN
	newAction( Window::tr( "&Konsola" ), this, QKeySequence( "Ctlr+Shift+C" ), console, false, QIcon(), true );
	addSeparator();
#endif
	newAction( QString(), this, QKeySequence( "`" ), toggleVisibility, false, QIcon(), false );
	newAction( Window::tr( "&Pełny ekran" ), this, QKeySequence( "F" ), toggleFullScreen, false, QMPlay2Core.getIconFromTheme( "view-fullscreen" ), false );
	newAction( Window::tr( "&Tryb kompaktowy" ), this, QKeySequence( "Alt+V" ), toggleCompactView, false, QIcon(), true  );
	addSeparator();
	newAction( Window::tr( "&Zamknij" ), this, QKeySequence( "Alt+F4" ), close, false, QMPlay2Core.getIconFromTheme( "application-exit" ), false );
}

MenuBar::Widgets::Widgets( MenuBar *parent, const QString &name ) :
	QMenu( name, parent )
{}
void MenuBar::Widgets::menuShow()
{
	clear();
	QMenu *menu = qobject_cast< QMainWindow * >( QMPlay2GUI.mainW )->createPopupMenu();
	if ( menu )
	{
		foreach ( QAction *act, menu->actions() )
		{
			if ( act->parent() == menu )
				act->setParent( this );
			addAction( act );
		}
		menu->deleteLater();
	}
}

MenuBar::Playlist::Playlist( MenuBar *parent, const QString &name ) : QMenu( name, parent )
{
	add = new Add( this );
	addMenu( add );

	addSeparator();
	newAction( Playlist::tr( "&Zatrzymaj ładowanie" ), this, QKeySequence( "F4" ), stopLoading, false, QMPlay2Core.getIconFromTheme( "process-stop" ), false );
	addSeparator();
	newAction( Playlist::tr( "&Synchronizuj katalog" ), this, QKeySequence( "F5" ), sync, false, QMPlay2Core.getIconFromTheme( "view-refresh" ), false );
	addSeparator();
	newAction( Playlist::tr( "Załaduj l&istę" ), this, QKeySequence( "Ctrl+L" ), loadPlist, false, QIcon(), false );
	newAction( Playlist::tr( "Zapisz &listę" ), this, QKeySequence( "Ctrl+S" ), savePlist, false, QIcon(), false );
	addSeparator();
	newAction( Playlist::tr( "&Usuń zaznaczone wpisy" ), this, QKeySequence( "Del" ), delEntries, true, QMPlay2Core.getIconFromTheme( "list-remove" ), false );
	newAction( Playlist::tr( "U&suń wpisy bez grupy" ), this, QKeySequence( "Ctrl+Del" ), delNonGroupEntries, false, QMPlay2Core.getIconFromTheme( "list-remove" ), false );
	newAction( Playlist::tr( "&Wyczyść listę" ), this, QKeySequence( "Shift+Del" ), clear, false, QMPlay2Core.getIconFromTheme( "archive-remove" ), false );
	addSeparator();
	newAction( Playlist::tr( "K&opiuj" ), this, QKeySequence( "Ctrl+C" ), copy, false, QMPlay2Core.getIconFromTheme( "edit-copy" ), false );
	newAction( Playlist::tr( "Wkl&ej" ), this, QKeySequence( "Ctrl+V" ), paste, false, QMPlay2Core.getIconFromTheme( "edit-paste" ), false );
	addSeparator();

	extensions = new QMenu( Playlist::tr( "&Rozszerzenia" ), this );
	extensions->setEnabled( false );
	addMenu( extensions );

	addSeparator();
	newAction( Playlist::tr( "U&twórz grupę" ), this, QKeySequence( "F7" ), newGroup, false, QMPlay2Core.getIconFromTheme( "folder-new" ), false );
	newAction( Playlist::tr( "Z&mień nazwę" ), this, QKeySequence( "F2" ), renameGroup, false, QIcon(), false );
	addSeparator();
	newAction( Playlist::tr( "Z&najdź wpisy" ), this, QKeySequence( "Ctrl+F" ), find, false, QMPlay2Core.getIconFromTheme( "edit-find" ), false );
	addSeparator();

	sort = new Sort( this );
	addMenu( sort );

	addSeparator();
	newAction( Playlist::tr( "&Zwiń wszystkie" ), this, QKeySequence(), collapseAll, false, QIcon(), false );
	newAction( Playlist::tr( "&Rozwiń wszystkie" ), this, QKeySequence(), expandAll, false, QIcon(), false );
	addSeparator();
	newAction( Playlist::tr( "&Idź do odtwarzanego" ), this, QKeySequence( "Ctrl+P" ), goToPlaying, false, QIcon(), false );
	addSeparator();
	newAction( Playlist::tr( "&Kolejkuj" ), this, QKeySequence( "Q" ), queue, false, QIcon(), false );
	addSeparator();
	newAction( Playlist::tr( "&Właściwości" ), this, QKeySequence( "Alt+Return" ), entryProperties, false, QMPlay2Core.getIconFromTheme( "document-properties" ), false );
}
MenuBar::Playlist::Add::Add( QMenu *parent, const QString &name ) : QMenu( name, parent )
{
	setIcon( QMPlay2Core.getIconFromTheme( "list-add" ) );
	newAction( Add::tr( "&Pliki" ), this, QKeySequence( "Ctrl+I" ), file, false, QMPlay2Core.getIconFromTheme( "applications-multimedia" ), false );
	newAction( Add::tr( "&Katalog" ), this, QKeySequence( "Ctrl+D" ), dir, false, QMPlay2Core.getIconFromTheme( "folder" ), false );
	newAction( Add::tr( "&Adres" ), this, QKeySequence( "Ctrl+U" ), address, false, QMPlay2Core.getIconFromTheme( "application-x-mswinurl" ), false );
	addSeparator();
}
MenuBar::Playlist::Sort::Sort( QMenu *parent, const QString &name ) : QMenu( name, parent )
{
	newAction( Sort::tr( "&Od najkrótszej do najdłuższej" ), this, QKeySequence(), timeSort1, false, QIcon(), false );
	newAction( Sort::tr( "O&d najdłuższej do najkrótszej" ), this, QKeySequence(), timeSort2, false, QIcon(), false );
	addSeparator();
	newAction( Sort::tr( "&A-Z" ), this, QKeySequence(), titleSort1, false, QIcon(), false );
	newAction( Sort::tr( "&Z-A" ), this, QKeySequence(), titleSort2, false, QIcon(), false );
}

MenuBar::Player::Player( MenuBar *parent, const QString &name ) : QMenu( name, parent )
{
	newAction( QString(), this, QKeySequence( "Space" ), togglePlay, false, QMPlay2Core.getIconFromTheme( "media-playback-start" ), false );
	newAction( Player::tr( "&Zatrzymaj" ), this, QKeySequence( "V" ), stop, false, QMPlay2Core.getIconFromTheme( "media-playback-stop" ), false );
	newAction( Player::tr( "&Następny" ), this, QKeySequence( "B" ), next, true, QMPlay2Core.getIconFromTheme( "media-skip-forward" ), false );
	newAction( Player::tr( "&Poprzedni" ), this, QKeySequence( "Z" ), prev, true, QMPlay2Core.getIconFromTheme( "media-skip-backward" ), false );
	newAction( Player::tr( "Następna &klatka" ), this, QKeySequence( "." ), nextFrame, true, QIcon(), false );
	addSeparator();

	repeat = new Repeat( this );
	addMenu( repeat );

	addSeparator();
	newAction( Player::tr( "Przewiń do p&rzodu" ), this, QKeySequence( "Right" ), seekF, true, QIcon(), false );
	newAction( Player::tr( "Przewiń do &tyłu" ), this, QKeySequence( "Left" ), seekB, true, QIcon(), false );
	newAction( Player::tr( "Przewiń dużo do pr&zodu" ), this, QKeySequence( "Up" ), lSeekF, true, QIcon(), false );
	newAction( Player::tr( "Przewiń dużo do t&yłu" ), this, QKeySequence( "Down" ), lSeekB, true, QIcon(), false );
	addSeparator();
	newAction( Player::tr( "Szy&bciej" ), this, QKeySequence( "]" ), speedUp, true, QIcon(), false );
	newAction( Player::tr( "Wo&lniej" ), this, QKeySequence( "[" ), slowDown, true, QIcon(), false );
	newAction( Player::tr( "&Ustaw szybkość" ), this, QKeySequence( "Shift+S" ), setSpeed, false, QIcon(), false );
	addSeparator();
	newAction( Player::tr( "Pow&iększ obraz" ), this, QKeySequence( "E" ), zoomIn, true, QIcon(), false );
	newAction( Player::tr( "Po&mniejsz obraz" ), this, QKeySequence( "W" ), zoomOut, true, QIcon(), false );
	newAction( Player::tr( "Przełącz &współczynnik proporcji" ), this, QKeySequence( "A" ), switchARatio, true, QIcon(), false );

	aRatio = new AspectRatio( this );
	addMenu( aRatio );

	newAction( Player::tr( "Resetuj ustawienia obr&azu" ), this, QKeySequence( "R" ), reset, false, QIcon(), false );

	addSeparator();
	newAction( Player::tr( "&Głośniej" ), this, QKeySequence( "*" ), volUp, true, QIcon(), false );
	newAction( Player::tr( "&Ciszej" ), this, QKeySequence( "/" ), volDown, true, QIcon(), false );
	newAction( Player::tr( "&Wyciszenie" ), this, QKeySequence( "M" ), toggleMute, false, QMPlay2Core.getIconFromTheme( "audio-volume-muted" ), true );
}
MenuBar::Player::Repeat::Repeat( QMenu *parent, const QString &name ) : QMenu( name, parent )
{
	choice = new QActionGroup( this );
	choice->addAction( newAction( Repeat::tr( "&Bez zapętlenia" ), this, QKeySequence( "Alt+0" ), normal, false, QIcon(), true ) );
	addSeparator();
	choice->addAction( newAction( Repeat::tr( "Zapętlenie &utworu" ), this, QKeySequence( "Alt+1" ), repeatEntry, false, QIcon(), true ) );
	choice->addAction( newAction( Repeat::tr( "Zapętlenie &grupy" ), this, QKeySequence( "Alt+2" ), repeatGroup, false, QIcon(), true ) );
	choice->addAction( newAction( Repeat::tr( "Zapętlenie &listy" ), this, QKeySequence( "Alt+3" ), repeatList, false, QIcon(), true ) );
	choice->addAction( newAction( Repeat::tr( "Losowe &odtwarzanie" ), this, QKeySequence( "Alt+4" ), random, false, QIcon(), true ) );
	choice->addAction( newAction( Repeat::tr( "Losowe odtwarzanie g&rupy" ), this, QKeySequence( "Alt+5" ), randomGroup, false, QIcon(), true ) );

	normal->setObjectName( "normal" );
	repeatEntry->setObjectName( "repeatEntry" );
	repeatGroup->setObjectName( "repeatGroup" );
	repeatList->setObjectName( "repeatList" );
	random->setObjectName( "random" );
	randomGroup->setObjectName( "randomGroup" );
}
MenuBar::Player::AspectRatio::AspectRatio( QMenu *parent, const QString &name ) : QMenu( name, parent )
{
	choice = new QActionGroup( this );
	choice->addAction( newAction( AspectRatio::tr( "&Auto" ), this, QKeySequence(), _auto, false, QIcon(), true ) );
	addSeparator();
	choice->addAction( newAction( "&1:1", this, QKeySequence(), _1x1, false, QIcon(), true ) );
	choice->addAction( newAction( "&4:3", this, QKeySequence(), _4x3, false, QIcon(), true ) );
	choice->addAction( newAction( "&5:4", this, QKeySequence(), _5x4, false, QIcon(), true ) );
	choice->addAction( newAction( "&16:9", this, QKeySequence(), _16x9, false, QIcon(), true ) );
	choice->addAction( newAction( "&3:2", this, QKeySequence(), _3x2, false, QIcon(), true ) );
	choice->addAction( newAction( "&21:9", this, QKeySequence(), _21x9, false, QIcon(), true ) );
	addSeparator();
	choice->addAction( newAction( AspectRatio::tr( "&Zależne od rozmiaru" ), this, QKeySequence(), sizeDep, false, QIcon(), true ) );
	choice->addAction( newAction( AspectRatio::tr( "&Wyłączone" ), this, QKeySequence(), off, false, QIcon(), true ) );

	_auto->setObjectName( "auto" );
	_1x1->setObjectName( "1" );
	_4x3->setObjectName( QString::number( 4.0 / 3.0 ) );
	_5x4->setObjectName( QString::number( 5.0 / 4.0 ) );
	_16x9->setObjectName( QString::number( 16.0 / 9.0 ) );
	_3x2->setObjectName( QString::number( 3.0 / 2.0 ) );
	_21x9->setObjectName( QString::number( 21.0 / 9.0 ) );
	sizeDep->setObjectName( "sizeDep" );
	off->setObjectName( "off" );
}
void MenuBar::Player::seekActionsEnable( bool e )
{
	Qt::ShortcutContext ctx = e ? Qt::WindowShortcut : Qt::WidgetShortcut;
	seekF->setShortcutContext( ctx );
	seekB->setShortcutContext( ctx );
	lSeekF->setShortcutContext( ctx );
	lSeekB->setShortcutContext( ctx );
}

MenuBar::Playing::Playing( MenuBar *parent, const QString &name ) : QMenu( name, parent )
{
	newAction( Playing::tr( "&Dźwięk włączony" ), this, QKeySequence( "D" ), toggleAudio, false, QIcon(), true )->setObjectName( "toggleAudio" );
	toggleAudio->setChecked( true );

	audioChannels = new AudioChannels( this );
	addMenu( audioChannels );

	addSeparator();

	newAction( Playing::tr( "O&braz włączony" ), this, QKeySequence( "O" ), toggleVideo, false, QIcon(), true )->setObjectName( "toggleVideo" );
	toggleVideo->setChecked( true );

	videoFilters = new VideoFilters( this );
	addMenu( videoFilters );

	newAction( Playing::tr( "Ustaw opóźn&ienie obrazu" ), this, QKeySequence( "Shift+O" ), videoSync, false, QIcon(), false );
	newAction( Playing::tr( "&Opóźnij obraz" ) + " (100ms)", this, QKeySequence( "-" ), slowDownVideo, true, QIcon(), false );
	newAction( Playing::tr( "&Przyspiesz obraz" ) + " (100ms)", this, QKeySequence( "+" ), speedUpVideo, true, QIcon(), false );
	addSeparator();
	newAction( Playing::tr( "&Napisy włączone" ), this, QKeySequence( "N" ), toggleSubtitles, false, QIcon(), true )->setObjectName( "toggleSubtitles" );
	toggleSubtitles->setChecked( true );
	newAction( Playing::tr( "Dodaj n&apisy z pliku" ), this, QKeySequence( "Alt+I" ), subsFromFile, false, QIcon(), false );
	newAction( Playing::tr( "Ustaw opóźn&ienie napisów" ), this, QKeySequence( "Shift+N" ), subtitlesSync, true, QIcon(), false );
	newAction( Playing::tr( "&Opóźnij napisy" ) + " (100ms)", this, QKeySequence( "Shift+Z" ), slowDownSubtitles, true, QIcon(), false );
	newAction( Playing::tr( "&Przyspiesz napisy" ) + " (100ms)", this, QKeySequence( "Shift+X" ), speedUpSubtitles, true, QIcon(), false );
	newAction( Playing::tr( "Po&większ napisy" ), this, QKeySequence( "Shift+R" ), biggerSubtitles, true, QIcon(), false );
	newAction( Playing::tr( "Po&mniejsz napisy" ), this, QKeySequence( "Shift+T" ), smallerSubtitles, true, QIcon(), false );
	addSeparator();
	newAction( Playing::tr( "&Ustawienia odtwarzania" ), this, QKeySequence( "Ctrl+Shift+P" ), playingSettings, true, QMPlay2Core.getIconFromTheme( "configure" ), false );
	addSeparator();
	newAction( Playing::tr( "&Zrzut ekranu" ), this, QKeySequence( "Alt+S" ), screenShot, true, QIcon(), false );
}
MenuBar::Playing::VideoFilters::VideoFilters( QMenu *parent, const QString &name ) : QMenu( name, parent )
{
	/** Korektor wideo */
	QMenu *videoEqualizerMenu = new QMenu( VideoFilters::tr( "&Korektor wideo" ) );
	addMenu( videoEqualizerMenu );
	QWidgetAction *widgetAction = new QWidgetAction( this );
	widgetAction->setDefaultWidget( videoEqualizer = new VideoEqualizer );
	videoEqualizerMenu->addAction( widgetAction );
	/**/
	newAction( VideoFilters::tr( "&Więcej filtrów" ), this, QKeySequence( "Alt+F" ), more, false, QIcon(), false );
	newAction( VideoFilters::tr( "Odbicie &lustrzane" ), this, QKeySequence( "Ctrl+M" ), hFlip, true, QIcon(), true );
	newAction( VideoFilters::tr( "Odbicie &pionowe" ), this, QKeySequence( "Ctrl+R" ), vFlip, true, QIcon(), true );
}
MenuBar::Playing::AudioChannels::AudioChannels( QMenu *parent, const QString &name ) : QMenu( name, parent )
{
	choice = new QActionGroup( this );
	choice->addAction( newAction( AudioChannels::tr( "&Automatycznie" ), this, QKeySequence(), _auto, false, QIcon(), true ) );
	addSeparator();
	choice->addAction( newAction( AudioChannels::tr( "&Mono" ), this, QKeySequence(), _1, false, QIcon(), true ) );
	choice->addAction( newAction( AudioChannels::tr( "&Stereo" ), this, QKeySequence(), _2, false, QIcon(), true ) );
	choice->addAction( newAction( "&4.0", this, QKeySequence(), _4, false, QIcon(), true ) );
	choice->addAction( newAction( "&5.1", this, QKeySequence(), _6, false, QIcon(), true ) );
	choice->addAction( newAction( "&7.1", this, QKeySequence(), _8, false, QIcon(), true ) );
	addSeparator();
	choice->addAction( newAction( AudioChannels::tr( "&Inne" ), this, QKeySequence(), other, false, QIcon(), true ) );

	_auto->setObjectName( "auto" );
	_1->setObjectName( "1" );
	_2->setObjectName( "2" );
	_4->setObjectName( "4" );
	_6->setObjectName( "6" );
	_8->setObjectName( "8" );
	other->setEnabled( false );
}

MenuBar::Options::Options( MenuBar *parent, const QString &name ) : QMenu( name, parent )
{
	newAction( Options::tr( "&Ustawienia" ), this, QKeySequence( "Ctrl+O" ), settings, false, QMPlay2Core.getIconFromTheme( "configure" ), false );
	newAction( Options::tr( "Ustawienia &modułów" ), this, QKeySequence( "Ctrl+Shift+O" ), modulesSettings, false, QMPlay2Core.getIconFromTheme( "configure" ), false );
	addSeparator();
	newAction( Options::tr( "&Pokaż ikonkę w tray'u" ), this, QKeySequence( "Ctrl+T" ), trayVisible, false, QIcon(), true );
}

MenuBar::Help::Help( MenuBar *parent, const QString &name ) : QMenu( name, parent )
{
	newAction( Help::tr( "&O QMPlay2" ), this, QKeySequence( "F1" ), about, false, QIcon(), false );
	newAction( Help::tr( "&Aktualizacje" ), this, QKeySequence( "F12" ), updates, false, QIcon(), false );
	addSeparator();
	newAction( Help::tr( "O &Qt" ), this, QKeySequence(), aboutQt, false, QIcon(), false );
}

void MenuBar::widgetsMenuShow()
{
	widgets->menuShow();
}
