#include <SettingsWidget.hpp>

#include <OSDSettingsW.hpp>
#include <DeintSettingsW.hpp>
#include <OtherVFiltersW.hpp>
#include <Main.hpp>

#if QT_VERSION < 0x050000
	#include <QDesktopServices>
#else
	#include <QStandardPaths>
#endif
#include <QNetworkProxy>
#include <QStyleFactory>
#include <QRadioButton>
#include <QApplication>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QScrollArea>
#include <QToolButton>
#include <QGridLayout>
#include <QMainWindow>
#include <QTextCodec>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QDebug>
#include <QDir>

#include <Appearance.hpp>
#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Module.hpp>

#if !defined( Q_OS_WIN ) && !defined( Q_OS_MAC )
	#define ICONS_FROM_THEME
#endif

class Page1 : public QWidget
{
public:
	QGridLayout *layout;
	QLabel *langL, *styleL, *encodingL, *screenshotL;
	QComboBox *langBox, *styleBox, *encodingB, *screenshotFormatB;
	QLineEdit *screenshotE;
	QPushButton *setAppearanceB;
#ifdef ICONS_FROM_THEME
	QCheckBox *iconsFromTheme;
#endif
	QCheckBox *showCoversB, *showDirCoversB, *autoOpenVideoWindowB, *autoUpdatesB, *tabsNorths, *allowOnlyOneInstance;
	QToolButton *screenshotB;
	QPushButton *clearCoversCache, *resetSettingsB;
	QGroupBox *proxyB, *proxyLoginB;
	QGridLayout *proxyL;
	QVBoxLayout *proxyLoginL;
	QLineEdit *proxyUserE, *proxyPasswordE, *proxyHostE;
	QSpinBox *proxyPortB;
};
class Page2 : public QWidget
{
public:
	QWidget *w;
	QScrollArea *scrollA;
	QGridLayout *layout1;
	QGridLayout *layout2;
	QLabel *shortSeekL, *longSeekL, *bufferLocalL, *bufferNetworkL, *playIfBufferedL, *maxVolL;
	QSpinBox *shortSeekB, *longSeekB, *bufferLocalB, *bufferNetworkB, *samplerateB, *channelsB, *maxVolB;
	QDoubleSpinBox *playIfBufferedB;
	QCheckBox *forceSamplerate, *forceChannels, *showBufferedTimeOnSlider, *savePos, *keepZoom, *keepARatio, *syncVtoA, *keepSubtitlesDelay, *keepSubtitlesScale, *keepVideoDelay, *keepSpeed, *silence, *scrollSeek, *restoreVideoEq;

	QGroupBox *replayGain;
	QVBoxLayout *replayGainL;
	QCheckBox *replayGainAlbum, *replayGainPreventClipping;
	QDoubleSpinBox *replayGainPreamp;

	QListWidget *modulesList[ 3 ];
};
class Page3 : public QWidget
{
public:
	QHBoxLayout *layout;
	QListWidget *listW;
	QScrollArea *scrollA;
	Module *module;
};
class Page4 : public OSDSettingsW
{
public:
	Page4() :
		OSDSettingsW( "Subtitles" ) {}

	QGridLayout *layout2;
	QGroupBox *toASSGB;
	QCheckBox *colorsAndBordersB, *marginsAndAlignmentB, *fontsB, *overridePlayResB;
};
class Page5 : public OSDSettingsW
{
public:
	Page5() :
		OSDSettingsW( "OSD" ) {}

	QCheckBox *enabledB;
};
class Page6 : public QWidget
{
public:
	QGridLayout *layout;
	DeintSettingsW *deintSettingsW;
	QLabel *otherVFiltersL;
	OtherVFiltersW *otherVFiltersW;
};

static inline void AddVHSpacer( QGridLayout &layout )
{
	layout.addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ), layout.rowCount(), 0 ); //vSpacer
	layout.addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum ), 0, layout.columnCount() ); //hSpacer
}

/**/

void SettingsWidget::InitSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.init( "SubtitlesEncoding", "UTF-8" );
#if QT_VERSION < 0x050000
	QMPSettings.init( "screenshotPth", QDesktopServices::storageLocation( QDesktopServices::PicturesLocation ) );
#else
	QMPSettings.init( "screenshotPth", QStandardPaths::standardLocations( QStandardPaths::PicturesLocation ) );
#endif
#ifdef Q_OS_WIN
	QMPSettings.init( "screenshotFormat", ".bmp" );
#else
	QMPSettings.init( "screenshotFormat", ".ppm" );
#endif
#ifdef ICONS_FROM_THEME
	QMPSettings.init( "IconsFromTheme", true );
#else
	QMPSettings.init( "IconsFromTheme", false );
#endif
	QMPSettings.init( "ShowCovers", true );
	QMPSettings.init( "ShowDirCovers", true );
	QMPSettings.init( "AutoOpenVideoWindow", true );
	if ( !QMPSettings.contains( "AutoUpdates" ) )
		QMPSettings.init( "AutoUpdates", !QFile::exists( QMPlay2Core.getQMPlay2Dir() + "noautoupdates" ) );
	QMPSettings.init( "MainWidget/TabPositionNorth", false );
	QMPSettings.init( "AllowOnlyOneInstance", false );
	QMPSettings.init( "Proxy/Use", false );
	QMPSettings.init( "Proxy/Host", QString() );
	QMPSettings.init( "Proxy/Port", 80 );
	QMPSettings.init( "Proxy/Login", false );
	QMPSettings.init( "Proxy/User", QString() );
	QMPSettings.init( "Proxy/Password", QString() );
	QMPSettings.init( "ShortSeek", 5 );
	QMPSettings.init( "LongSeek", 30 );
	QMPSettings.init( "AVBufferLocal", 75 );
	QMPSettings.init( "AVBufferNetwork", 7500 );
	QMPSettings.init( "PlayIfBuffered", 1.75 );
	QMPSettings.init( "MaxVol", 100 );
	QMPSettings.init( "Volume", 100 );
	QMPSettings.init( "ForceSamplerate", false );
	QMPSettings.init( "Samplerate", 48000 );
	QMPSettings.init( "ForceChannels", false );
	QMPSettings.init( "Channels", 2 );
	QMPSettings.init( "ReplayGain/Enabled", false );
	QMPSettings.init( "ReplayGain/Album", false );
	QMPSettings.init( "ReplayGain/PreventClipping", true );
	QMPSettings.init( "ReplayGain/Preamp", 0.0 );
	QMPSettings.init( "ShowBufferedTimeOnSlider", true );
	QMPSettings.init( "SavePos", false );
	QMPSettings.init( "KeepZoom", false );
	QMPSettings.init( "KeepARatio", false );
	QMPSettings.init( "KeepSubtitlesDelay", false );
	QMPSettings.init( "KeepSubtitlesScale", false );
	QMPSettings.init( "KeepVideoDelay", false );
	QMPSettings.init( "KeepSpeed", false );
	QMPSettings.init( "SyncVtoA", true );
	QMPSettings.init( "Silence", true );
	QMPSettings.init( "ScrollSeek", false );
	QMPSettings.init( "RestoreVideoEqualizer", false );
	QMPSettings.init( "ApplyToASS/ColorsAndBorders", true );
	QMPSettings.init( "ApplyToASS/MarginsAndAlignment", false );
	QMPSettings.init( "ApplyToASS/FontsAndSpacing", false );
	QMPSettings.init( "ApplyToASS/OverridePlayRes", false );
	QMPSettings.init( "ApplyToASS/ApplyToASS", false );
	QMPSettings.init( "OSD/Enabled", true );
	OSDSettingsW::init( "Subtitles", 20, 0, 15, 15, 15, 7, 1.5, 1.5, QColor( 0xFF, 0xA8, 0x58, 0xFF ), Qt::black, Qt::black );
	OSDSettingsW::init( "OSD",       32, 0, 0,  0,  0,  4, 1.5, 1.5, QColor( 0xAA, 0xFF, 0x55, 0xFF ), Qt::black, Qt::black );
	DeintSettingsW::init();
	applyProxy();
}
void SettingsWidget::SetAudioChannelsMenu()
{
	const bool forceChn = QMPlay2Core.getSettings().getBool( "ForceChannels" );
	const int chn = QMPlay2Core.getSettings().getInt( "Channels" );
	bool audioChannelsChecked = false;
	foreach ( QAction *act, QMPlay2GUI.menubar->playing->audioChannels->actions() )
		if ( ( !forceChn && act->objectName() == "auto" ) || ( forceChn && chn == act->objectName().toInt() ) )
		{
			act->setChecked( true );
			audioChannelsChecked = true;
		}
	if ( !audioChannelsChecked )
		QMPlay2GUI.menubar->playing->audioChannels->other->setChecked( true );
}
void SettingsWidget::SetAudioChannels( int chn )
{
	const bool forceChannels = chn >= 1 && chn <= 8;
	if ( forceChannels )
		QMPlay2Core.getSettings().set( "Channels", chn );
	QMPlay2Core.getSettings().set( "ForceChannels", forceChannels );
}

SettingsWidget::SettingsWidget( QWidget *p, int page, const QString &moduleName ) :
	QWidget( p ),
	wasShow( false ),
	moduleIndex( 0 )
{
	setWindowFlags( Qt::Window );

	Settings &QMPSettings = QMPlay2Core.getSettings();
	int idx, layout_row;

	setWindowTitle( tr( "Ustawienia" ) );
	setAttribute( Qt::WA_DeleteOnClose );

	tabW = new QTabWidget;
	page1 = new Page1;
	page2 = new Page2;
	page3 = new Page3;
	page4 = new Page4;
	page5 = new Page5;
	page6 = new Page6;
	tabW->addTab( page1, tr( "Ustawienia ogólne" ) );
	tabW->addTab( page2, tr( "Ustawienia odtwarzania" ) );
	tabW->addTab( page3, tr( "Moduły" ) );
	tabW->addTab( page4, tr( "Napisy" ) );
	tabW->addTab( page5, tr( "OSD" ) );
	tabW->addTab( page6, tr( "Filtry wideo" ) );

	applyB = new QPushButton;
	applyB->setText( tr( "Zastosuj" ) );
	connect( applyB, SIGNAL( clicked() ), this, SLOT( apply() ) );

	closeB = new QPushButton;
	closeB->setText( tr( "Zamknij" ) );
	closeB->setShortcut( QKeySequence( "Escape" ) );
	connect( closeB, SIGNAL( clicked() ), this, SLOT( close() ) );

	layout = new QGridLayout( this );
	layout->addWidget( tabW, 0, 0, 1, 3 );
	layout->addWidget( applyB, 1, 1, 1, 1 );
	layout->addWidget( closeB, 1, 2, 1, 1 );
	layout->setMargin( 3 );

	/* Strona 1 */
	page1->langL = new QLabel;
	page1->langL->setText( tr( "Język" ) + ": " );
	page1->langBox = new QComboBox;
	page1->langBox->addItem( "Polski", "pl" );
	page1->langBox->setCurrentIndex( 0 );
	QStringList langs = QMPlay2GUI.getLanguages();
	for ( int i = 0 ; i < langs.count() ; i++ )
	{
		page1->langBox->addItem( QMPlay2GUI.getLongFromShortLanguage( langs[ i ] ), langs[ i ] );
		if ( QMPlay2GUI.lang == langs[ i ] )
			page1->langBox->setCurrentIndex( i + 1 );
	}

	page1->styleL = new QLabel;
	page1->styleL->setText( tr( "Styl" ) + ": " );
	page1->styleBox = new QComboBox;
	page1->styleBox->addItems( QStyleFactory::keys() );
	idx = page1->styleBox->findText( qApp->style()->objectName(), Qt::MatchFixedString );
	if ( idx > -1 && idx < page1->styleBox->count() )
		page1->styleBox->setCurrentIndex( idx );
	connect( page1->styleBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( chStyle() ) );

	page1->encodingL = new QLabel( tr( "Kodowanie napisów" ) + ": " );
	page1->encodingB = new QComboBox;
	QStringList encodings;
	foreach ( QByteArray item, QTextCodec::availableCodecs() )
		encodings += QTextCodec::codecForName( item )->name();
	encodings.removeDuplicates();
	page1->encodingB->addItems( encodings );
	idx = page1->encodingB->findText( QMPSettings.getByteArray( "SubtitlesEncoding" ) );
	if ( idx > -1 )
		page1->encodingB->setCurrentIndex( idx );

	page1->screenshotL = new QLabel( tr( "Ścieżka do zrzutów ekranu" ) + ": " );
	page1->screenshotE = new QLineEdit;
	page1->screenshotE->setReadOnly( true );
	page1->screenshotE->setText( QMPSettings.getString( "screenshotPth" ) );
	page1->screenshotFormatB = new QComboBox;
	page1->screenshotFormatB->addItems( QStringList() << ".ppm" << ".bmp" << ".png" );
	page1->screenshotFormatB->setCurrentIndex( page1->screenshotFormatB->findText( QMPSettings.getString( "screenshotFormat" ) ) );
	page1->screenshotB = new QToolButton;
	page1->screenshotB->setIcon( QMPlay2Core.getIconFromTheme( "folder-open" ) );
	page1->screenshotB->setToolTip( tr( "Przeglądaj" ) );
	connect( page1->screenshotB, SIGNAL( clicked() ), this, SLOT( chooseScreenshotDir() ) );

	page1->setAppearanceB = new QPushButton( tr( "Ustaw wygląd" ) );
	connect( page1->setAppearanceB, SIGNAL( clicked() ), this, SLOT( setAppearance() ) );

#ifdef ICONS_FROM_THEME
	page1->iconsFromTheme = new QCheckBox( tr( "Użyj systemowego zestawu ikon" ) );
	page1->iconsFromTheme->setChecked( QMPSettings.getBool( "IconsFromTheme" ) );
#endif

	page1->showCoversB = new QCheckBox( tr( "Wyświetlaj okładki" ) );
	page1->showCoversB->setChecked( QMPSettings.getBool( "ShowCovers" ) );

	page1->showDirCoversB = new QCheckBox( tr( "Wyświetlaj okładki z katalogu jeżeli nie ma ich w pliku muzycznym" ) );
	page1->showDirCoversB->setChecked( QMPSettings.getBool( "ShowDirCovers" ) );
	connect( page1->showCoversB, SIGNAL( clicked( bool ) ), page1->showDirCoversB, SLOT( setEnabled( bool ) ) );
	page1->showDirCoversB->setEnabled( page1->showCoversB->isChecked() );

	page1->autoOpenVideoWindowB = new QCheckBox( tr( "Automatyczne otwieranie okienka z filmem" ) );
	page1->autoOpenVideoWindowB->setChecked( QMPSettings.getBool( "AutoOpenVideoWindow" ) );

	page1->autoUpdatesB = new QCheckBox( tr( "Automatycznie sprawdzaj i pobieraj aktualizacje" ) );
	page1->autoUpdatesB->setChecked( QMPSettings.getBool( "AutoUpdates" ) );

	page1->tabsNorths = new QCheckBox( tr( "Pokazuj karty w górnej części okna głównego" ) );
	page1->tabsNorths->setChecked( QMPSettings.getBool( "MainWidget/TabPositionNorth" ) );

	page1->allowOnlyOneInstance = new QCheckBox( tr( "Zezwalaj na tylko jedną instancję" ) );
	page1->allowOnlyOneInstance->setChecked( QMPSettings.getBool( "AllowOnlyOneInstance" ) );


	page1->proxyB = new QGroupBox( tr( "Używaj serwera proxy" ) );
	page1->proxyB->setCheckable( true );
	page1->proxyB->setChecked( QMPSettings.getBool( "Proxy/Use" ) );

	page1->proxyHostE = new QLineEdit( QMPSettings.getString( "Proxy/Host" ) );
	page1->proxyHostE->setPlaceholderText( tr( "Adres serwera proxy" ) );

	page1->proxyPortB = new QSpinBox;
	page1->proxyPortB->setRange( 1, 65535 );
	page1->proxyPortB->setValue( QMPSettings.getInt( "Proxy/Port" ) );
	page1->proxyPortB->setToolTip( tr( "Port serwera proxy" ) );

	page1->proxyLoginB = new QGroupBox( tr( "Serwer proxy wymaga logowania" ) );
	page1->proxyLoginB->setCheckable( true );
	page1->proxyLoginB->setChecked( QMPSettings.getBool( "Proxy/Login" ) );

	page1->proxyUserE = new QLineEdit( QMPSettings.getString( "Proxy/User" ) );
	page1->proxyUserE->setPlaceholderText( tr( "Nazwa użytkownika" ) );

	page1->proxyPasswordE = new QLineEdit( QByteArray::fromBase64( QMPSettings.getByteArray( "Proxy/Password" ) ) );
	page1->proxyPasswordE->setEchoMode( QLineEdit::Password );
	page1->proxyPasswordE->setPlaceholderText( tr( "Hasło" ) );

	page1->proxyLoginL = new QVBoxLayout( page1->proxyLoginB );
	page1->proxyLoginL->addWidget( page1->proxyUserE );
	page1->proxyLoginL->addWidget( page1->proxyPasswordE );

	page1->proxyL = new QGridLayout( page1->proxyB );
	page1->proxyL->addWidget( page1->proxyLoginB, 0, 0, 1, 2 );
	page1->proxyL->addWidget( page1->proxyHostE, 1, 0, 1, 1 );
	page1->proxyL->addWidget( page1->proxyPortB, 1, 1, 1, 1 );


	page1->clearCoversCache = new QPushButton;
	page1->clearCoversCache->setText( tr( "Wyczyść pamięć podręczną okładek" ) );
	page1->clearCoversCache->setIcon( QMPlay2Core.getIconFromTheme( "view-refresh" ) );
	connect( page1->clearCoversCache, SIGNAL( clicked() ), this, SLOT( clearCoversCache() ) );

	page1->resetSettingsB = new QPushButton;
	page1->resetSettingsB->setText( tr( "Wyczyść ustawienia programu" ) );
	page1->resetSettingsB->setIcon( QMPlay2Core.getIconFromTheme( "view-refresh" ) );
	connect( page1->resetSettingsB, SIGNAL( clicked() ), this, SLOT( resetSettings() ) );

	layout_row = 0;
	page1->layout = new QGridLayout( page1 );
	page1->layout->addWidget( page1->langL, layout_row, 0, 1, 1 );
	page1->layout->addWidget( page1->langBox, layout_row++, 1, 1, 3 );
	page1->layout->addWidget( page1->styleL, layout_row, 0, 1, 1 );
	page1->layout->addWidget( page1->styleBox, layout_row++, 1, 1, 3 );
	page1->layout->addWidget( page1->encodingL, layout_row, 0, 1, 1 );
	page1->layout->addWidget( page1->encodingB, layout_row++, 1, 1, 3 );
	page1->layout->addWidget( page1->screenshotL, layout_row, 0, 1, 1 );
	page1->layout->addWidget( page1->screenshotE, layout_row, 1, 1, 1 );
	page1->layout->addWidget( page1->screenshotFormatB, layout_row, 2, 1, 1 );
	page1->layout->addWidget( page1->screenshotB, layout_row++, 3, 1, 1 );
	page1->layout->addWidget( page1->setAppearanceB, layout_row, 1, 1, 3 );
#ifdef ICONS_FROM_THEME
	page1->layout->addWidget( page1->iconsFromTheme, layout_row, 0, 1, 1 );
#endif
	layout_row++;
	page1->layout->addWidget( page1->showCoversB, layout_row++, 0, 1, 4 );
	page1->layout->addWidget( page1->showDirCoversB, layout_row++, 0, 1, 4 );
	page1->layout->addWidget( page1->autoOpenVideoWindowB, layout_row++, 0, 1, 4 );
	page1->layout->addWidget( page1->autoUpdatesB, layout_row++, 0, 1, 4 );
	page1->layout->addWidget( page1->tabsNorths, layout_row++, 0, 1, 4 );
	page1->layout->addWidget( page1->allowOnlyOneInstance, layout_row++, 0, 1, 4 );
	page1->layout->addWidget( page1->proxyB, layout_row++, 0, 1, 4 );
	page1->layout->addWidget( page1->clearCoversCache, layout_row, 0, 1, 1 );
	page1->layout->addWidget( page1->resetSettingsB, layout_row++, 1, 1, 3 );
	page1->layout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ), page1->layout->rowCount(), 0 ); //vSpacer

	/* Strona 2 */
	page2->shortSeekL = new QLabel( tr( "Krótkie przewijanie (strzałki: w lewo i w prawo)" ) + ": " );
	page2->shortSeekB = new QSpinBox;
	page2->shortSeekB->setRange( 2, 20 );
	page2->shortSeekB->setSuffix( " " + tr( "sek" ) );
	page2->shortSeekB->setValue( QMPSettings.getInt( "ShortSeek" ) );

	page2->longSeekL = new QLabel( tr( "Długie przewijanie (strzałki: w górę i w dół)" ) + ": " );
	page2->longSeekB = new QSpinBox;
	page2->longSeekB->setRange( 30, 300 );
	page2->longSeekB->setSuffix( " " + tr( "sek" ) );
	page2->longSeekB->setValue( QMPSettings.getInt( "LongSeek" ) );

	page2->bufferLocalL = new QLabel( tr( "Rozmiar bufora dla lokalnych strumieni (minimalna ilość paczek A/V)" ) + ": " );
	page2->bufferLocalB = new QSpinBox;
	page2->bufferLocalB->setRange( 1, 1000 );
	page2->bufferLocalB->setValue( QMPSettings.getInt( "AVBufferLocal" ) );

	page2->bufferNetworkL = new QLabel( tr( "Rozmiar bufora dla internetowych strumieni (minimalna ilość paczek A/V)" ) + ": " );
	page2->bufferNetworkB = new QSpinBox;
	page2->bufferNetworkB->setRange( 100, 100000 );
	page2->bufferNetworkB->setValue( QMPSettings.getInt( "AVBufferNetwork" ) );

	page2->playIfBufferedL = new QLabel( tr( "Rozpocznij odtwarzanie strumienia internetowego jeżeli zostanie zbuforowane" ) + ": " );
	page2->playIfBufferedB = new QDoubleSpinBox;
	page2->playIfBufferedB->setRange( 0.0, 5.0 );
	page2->playIfBufferedB->setSingleStep( 0.25 );
	page2->playIfBufferedB->setSuffix( " " + tr( "sek" ) );
	page2->playIfBufferedB->setValue( QMPSettings.getDouble( "PlayIfBuffered" ) );

	page2->maxVolL = new QLabel( tr( "Maksymalna głośność" ) );
	page2->maxVolB = new QSpinBox;
	page2->maxVolB->setRange( 100, 1000 );
	page2->maxVolB->setSingleStep( 50 );
	page2->maxVolB->setSuffix( " %" );
	page2->maxVolB->setValue( QMPSettings.getInt( "MaxVol" ) );

	page2->forceSamplerate = new QCheckBox( tr( "Wymuś częstotliwość próbkowania" ) );
	page2->forceSamplerate->setChecked( QMPSettings.getBool( "ForceSamplerate" ) );
	connect( page2->forceSamplerate, SIGNAL( stateChanged( int ) ), this, SLOT( page2EnableOrDisable() ) );
	page2->samplerateB = new QSpinBox;
	page2->samplerateB->setRange( 4000, 192000 );
	page2->samplerateB->setValue( QMPSettings.getInt( "Samplerate" ) );

	page2->forceChannels = new QCheckBox( tr( "Wymuś konwersję kanałów" ) );
	page2->forceChannels->setChecked( QMPSettings.getBool( "ForceChannels" ) );
	connect( page2->forceChannels, SIGNAL( stateChanged( int ) ), this, SLOT( page2EnableOrDisable() ) );
	page2->channelsB = new QSpinBox;
	page2->channelsB->setRange( 1, 8 );
	page2->channelsB->setValue( QMPSettings.getInt( "Channels" ) );

	page2EnableOrDisable();


	page2->replayGain = new QGroupBox( tr( "Użyj wyrównywania głośności, jeżeli dostępne" ) );
	page2->replayGain->setCheckable( true );
	page2->replayGain->setChecked( QMPSettings.getBool( "ReplayGain/Enabled" ) );

	page2->replayGainL = new QVBoxLayout( page2->replayGain );

	page2->replayGainAlbum = new QCheckBox( tr( "Tryb albumu dla wyrównywania głośności" ) );
	page2->replayGainAlbum->setChecked( QMPSettings.getBool( "ReplayGain/Album" ) );

	page2->replayGainPreventClipping = new QCheckBox( tr( "Zapobiegaj przesterowaniu" ) );
	page2->replayGainPreventClipping->setChecked( QMPSettings.getBool( "ReplayGain/PreventClipping" ) );

	page2->replayGainPreamp = new QDoubleSpinBox;
	page2->replayGainPreamp->setRange( -15.0, 15.0 );
	page2->replayGainPreamp->setPrefix( "Wzmocnienie: " );
	page2->replayGainPreamp->setSuffix( " dB" );
	page2->replayGainPreamp->setValue( QMPSettings.getDouble( "ReplayGain/Preamp" ) );

	page2->replayGainL->addWidget( page2->replayGainAlbum );
	page2->replayGainL->addWidget( page2->replayGainPreventClipping );
	page2->replayGainL->addWidget( page2->replayGainPreamp );


	page2->showBufferedTimeOnSlider = new QCheckBox( tr( "Pokazuj wskaźnik zbuforowanych danych na pasku przewijania" ) );
	page2->showBufferedTimeOnSlider->setChecked( QMPSettings.getBool( "ShowBufferedTimeOnSlider" ) );

	page2->savePos = new QCheckBox( tr( "Zapamiętaj pozycję odtwarzania" ) );
	page2->savePos->setChecked( QMPSettings.getBool( "SavePos" ) );

	page2->keepZoom = new QCheckBox( tr( "Pozostaw zoom między plikami wideo" ) );;
	page2->keepZoom->setChecked( QMPSettings.getBool( "KeepZoom" ) );

	page2->keepARatio = new QCheckBox( tr( "Pozostaw współczynnik proporcji między plikami wideo" ) );
	page2->keepARatio->setChecked( QMPSettings.getBool( "KeepARatio" ) );

	page2->keepSubtitlesDelay = new QCheckBox( tr( "Pozostaw opóźnienie napisów między plikami wideo" ) );
	page2->keepSubtitlesDelay->setChecked( QMPSettings.getBool( "KeepSubtitlesDelay" ) );

	page2->keepSubtitlesScale = new QCheckBox( tr( "Pozostaw powiększenie napisów między plikami wideo" ) );
	page2->keepSubtitlesScale->setChecked( QMPSettings.getBool( "KeepSubtitlesScale" ) );

	page2->keepVideoDelay = new QCheckBox( tr( "Pozostaw opóźnienie obrazu względem dźwięku między plikami wideo" ) );
	page2->keepVideoDelay->setChecked( QMPSettings.getBool( "KeepVideoDelay" ) );

	page2->keepSpeed = new QCheckBox( tr( "Pozostaw szybkość odtwarzania między plikami" ) );
	page2->keepSpeed->setChecked( QMPSettings.getBool( "KeepSpeed" ) );

	page2->syncVtoA = new QCheckBox( tr( "Synchronizuj obraz do dźwięku (pomijanie klatek)" ) );
	page2->syncVtoA->setChecked( QMPSettings.getBool( "SyncVtoA" ) );

	page2->silence = new QCheckBox( tr( "Wyciszaj podczas zatrzymywania odtwarzania" ) );
	page2->silence->setChecked( QMPSettings.getBool( "Silence" ) );

	page2->scrollSeek = new QCheckBox( tr( "Kółko myszki przewija muzykę/film" ) );
	page2->scrollSeek->setChecked( QMPSettings.getBool( "ScrollSeek" ) );

	page2->restoreVideoEq = new QCheckBox( tr( "Pamiętaj ustawienia korektora wideo" ) );
	page2->restoreVideoEq->setChecked( QMPSettings.getBool( "RestoreVideoEqualizer" ) );

	for ( int m = 0 ; m < 3 ; ++m )
	{
		page2->modulesList[ m ] = new QListWidget;
		connect( page2->modulesList[ m ], SIGNAL( itemDoubleClicked ( QListWidgetItem * ) ), this, SLOT( openModuleSettings( QListWidgetItem * ) ) );
		page2->modulesList[ m ]->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum ) );
		page2->modulesList[ m ]->setSelectionMode( QAbstractItemView::ExtendedSelection );
		page2->modulesList[ m ]->setDragDropMode( QAbstractItemView::InternalMove );
	}
	page2->modulesList[ 0 ]->setToolTip( tr( "Kolejność otwierania wyjścia obrazu" ) );
	page2->modulesList[ 1 ]->setToolTip( tr( "Kolejność otwierania wyjścia dźwięku" ) );
	page2->modulesList[ 2 ]->setToolTip( tr( "Kolejność otwierania dekoderów" ) );

	layout_row = 0;
	page2->w = new QWidget;
	page2->layout2 = new QGridLayout( page2->w );
	page2->layout2->addWidget( page2->shortSeekL, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->shortSeekB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->longSeekL, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->longSeekB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->bufferLocalL, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->bufferLocalB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->bufferNetworkL, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->bufferNetworkB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->playIfBufferedL, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->playIfBufferedB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->maxVolL, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->maxVolB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->forceSamplerate, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->samplerateB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->forceChannels, layout_row, 0, 1, 1 );
	page2->layout2->addWidget( page2->channelsB, layout_row++, 1, 1, 1 );
	page2->layout2->addWidget( page2->replayGain, layout_row++, 0, 1, 2 );
	page2->layout2->addWidget( page2->showBufferedTimeOnSlider, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->savePos, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->keepZoom, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->keepARatio, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->keepSubtitlesDelay, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->keepSubtitlesScale, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->keepVideoDelay, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->keepSpeed, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->syncVtoA, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->silence, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->scrollSeek, layout_row++, 0, 1, 3 );
	page2->layout2->addWidget( page2->restoreVideoEq, layout_row++, 0, 1, 3 );
	page2->layout2->setMargin( 0 );

	page2->scrollA = new QScrollArea;
	page2->scrollA->setFrameShape( QFrame::NoFrame );
	page2->scrollA->setWidget( page2->w );

	page2->layout1 = new QGridLayout( page2 );
	page2->layout1->addWidget( page2->scrollA, 0, 0, 1, 3 );
	page2->layout1->addWidget( page2->modulesList[ 0 ], 1, 0 );
	page2->layout1->addWidget( page2->modulesList[ 1 ], 1, 1 );
	page2->layout1->addWidget( page2->modulesList[ 2 ], 1, 2 );

	/* Strona 3 */
	page3->module = NULL;

	page3->listW = new QListWidget;
	page3->listW->setIconSize( QSize( 32, 32 ) );
	page3->listW->setMinimumSize( 200, 0 );
	page3->listW->setMaximumSize( 200, 16777215 );
	foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
	{
		QListWidgetItem *tWI = new QListWidgetItem( module->name() );
		tWI->setData( Qt::UserRole, qVariantFromValue( ( void * )module ) );
		QString toolTip = tr( "Zawiera" ) + ":";
		foreach ( Module::Info mod, module->getModulesInfo( true ) )
		{
			toolTip += "<p>&nbsp;&nbsp;&nbsp;&nbsp;";
			if ( !mod.imgPath().isEmpty() )
				toolTip += "<img width='22' height='22' src='" + mod.imgPath() + "'/> ";
			else
				toolTip += "- ";
			toolTip += mod.name + "</p>";
		}
		tWI->setToolTip( "<html>" + toolTip + "</html>" );
		tWI->setIcon( QMPlay2GUI.getIcon( module->image() ) );
		page3->listW->addItem( tWI );
		if ( page == 2 && !moduleName.isEmpty() && module->name() == moduleName )
			moduleIndex = page3->listW->count() - 1;
	}

	page3->scrollA = new QScrollArea;
	page3->scrollA->setWidgetResizable( true );
	page3->scrollA->setFrameShape( QFrame::NoFrame );

	page3->layout = new QHBoxLayout( page3 );
	page3->layout->addWidget( page3->listW );
	page3->layout->addWidget( page3->scrollA );
	connect( page3->listW, SIGNAL( currentItemChanged( QListWidgetItem *, QListWidgetItem * ) ), this, SLOT( chModule( QListWidgetItem * ) ) );

	/* Strona 4 */
	page4->colorsAndBordersB = new QCheckBox( tr( "Kolory i obramowania" ) );
	page4->colorsAndBordersB->setChecked( QMPSettings.getBool( "ApplyToASS/ColorsAndBorders" ) );

	page4->marginsAndAlignmentB = new QCheckBox( tr( "Marginesy i rozmieszczenie" ) );
	page4->marginsAndAlignmentB->setChecked( QMPSettings.getBool( "ApplyToASS/MarginsAndAlignment" ) );

	page4->fontsB = new QCheckBox( tr( "Czcionki i odstępy" ) );
	page4->fontsB->setChecked( QMPSettings.getBool( "ApplyToASS/FontsAndSpacing" ) );

	page4->overridePlayResB = new QCheckBox( tr( "Użyj jednakowej wielkości" ) );
	page4->overridePlayResB->setChecked( QMPSettings.getBool( "ApplyToASS/OverridePlayRes" ) );

	page4->toASSGB = new QGroupBox( tr( "Zastosuj dla napisów ASS/SSA" ) );
	page4->toASSGB->setCheckable( true );
	page4->toASSGB->setChecked( QMPSettings.getBool( "ApplyToASS/ApplyToASS" ) );

	page4->layout2 = new QGridLayout( page4->toASSGB );
	page4->layout2->addWidget( page4->colorsAndBordersB, 0, 0, 1, 1 );
	page4->layout2->addWidget( page4->marginsAndAlignmentB, 1, 0, 1, 1 );
	page4->layout2->addWidget( page4->fontsB, 0, 1, 1, 1 );
	page4->layout2->addWidget( page4->overridePlayResB, 1, 1, 1, 1 );

	page4->layout->addWidget( page4->toASSGB, 2, 0, 1, 5 );
	AddVHSpacer( *page4->layout );

	/* Strona 5 */
	page5->enabledB = new QCheckBox( tr( "OSD włączone" ) );
	page5->enabledB->setChecked( QMPSettings.getBool( "OSD/Enabled" ) );

	page5->layout->addWidget( page5->enabledB, 2, 0, 1, 5 );
	AddVHSpacer( *page5->layout );

	/* Strona 6 */
	page6->deintSettingsW = new DeintSettingsW;

	page6->otherVFiltersL = new QLabel( tr( "Programowe filtry wideo" ) + ":" );
	page6->otherVFiltersW = new OtherVFiltersW;
	connect( page6->otherVFiltersW, SIGNAL( itemDoubleClicked ( QListWidgetItem * ) ), this, SLOT( openModuleSettings( QListWidgetItem * ) ) );

	page6->layout = new QGridLayout( page6 );
	page6->layout->addWidget( page6->deintSettingsW, 0, 0, 1, 1 );
	page6->layout->addWidget( page6->otherVFiltersL, 1, 0, 1, 1 );
	page6->layout->addWidget( page6->otherVFiltersW, 2, 0, 1, 1 );


	connect( tabW, SIGNAL( currentChanged( int ) ), this, SLOT( tabCh( int ) ) );
	tabW->setCurrentIndex( page );

	show();
}

void SettingsWidget::setAudioChannels()
{
	page2->forceChannels->setChecked( QMPlay2Core.getSettings().getBool( "ForceChannels" ) );
	page2->channelsB->setValue( QMPlay2Core.getSettings().getInt( "Channels" ) );
}

void SettingsWidget::applyProxy()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QNetworkProxy proxy;
	if ( QMPSettings.getBool( "Proxy/Use" ) )
	{
		proxy.setType( QNetworkProxy::Socks5Proxy );
		proxy.setHostName( QMPSettings.getString( "Proxy/Host" ) );
		proxy.setPort( QMPSettings.getInt( "Proxy/Port" ) );
		if ( QMPSettings.getBool( "Proxy/Login" ) )
		{
			proxy.setUser( QMPSettings.getString( "Proxy/User" ) );
			proxy.setPassword( QByteArray::fromBase64( QMPSettings.getString( "Proxy/Password" ).toLatin1() ) );
		}
	}
	QNetworkProxy::setApplicationProxy( proxy );
}

void SettingsWidget::restartApp()
{
	QMPlay2GUI.restartApp = true;
	close();
	QMPlay2GUI.mainW->close();
}

void SettingsWidget::showEvent( QShowEvent * )
{
	if ( !wasShow )
	{
		QMPlay2GUI.restoreGeometry( "SettingsWidget/Geometry", this, QSize( 630, 425 ) );
		page3->listW->setCurrentRow( moduleIndex );
		wasShow = true;
	}
}
void SettingsWidget::closeEvent( QCloseEvent * )
{
	QMPlay2Core.getSettings().set( "SettingsWidget/Geometry", geometry() );
}

void SettingsWidget::chStyle()
{
	QString newStyle = page1->styleBox->currentText().toLower();
	if ( qApp->style()->objectName() != newStyle )
	{
		QMPlay2Core.getSettings().set( "Style", newStyle );
		QMPlay2GUI.setStyle();
	}
}
void SettingsWidget::apply()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	bool page3Restart = false;
	const int page = tabW->currentIndex() + 1;
	switch ( page )
	{
		case 1:
		{
			if ( QMPlay2GUI.lang != page1->langBox->itemData( page1->langBox->currentIndex() ).toString() )
			{
				QMPSettings.set( "Language", page1->langBox->itemData( page1->langBox->currentIndex() ).toString() );
				QMPlay2GUI.setLanguage();
				QMessageBox::information( this, tr( "Nowy język" ), tr( "Aby zastosować nowy język program zostanie uruchomiony ponownie!" ) );
				restartApp();
			}
#ifdef ICONS_FROM_THEME
			if ( page1->iconsFromTheme->isChecked() != QMPSettings.getBool( "IconsFromTheme" ) )
			{
				QMPSettings.set( "IconsFromTheme", page1->iconsFromTheme->isChecked() );
				if ( !QMPlay2GUI.restartApp )
				{
					QMessageBox::information( this, tr( "Zmiana ikon" ), tr( "Aby zastosować zmianę ikon program zostanie uruchomiony ponownie!" ) );
					restartApp();
				}
			}
#endif
			QMPSettings.set( "SubtitlesEncoding", page1->encodingB->currentText().toUtf8() );
			QMPSettings.set( "screenshotPth", page1->screenshotE->text() );
			QMPSettings.set( "screenshotFormat", page1->screenshotFormatB->currentText() );
			QMPSettings.set( "ShowCovers", page1->showCoversB->isChecked() );
			QMPSettings.set( "ShowDirCovers", page1->showDirCoversB->isChecked() );
			QMPSettings.set( "AutoOpenVideoWindow", page1->autoOpenVideoWindowB->isChecked() );
			QMPSettings.set( "AutoUpdates", page1->autoUpdatesB->isChecked() );
			QMPSettings.set( "MainWidget/TabPositionNorth", page1->tabsNorths->isChecked() );
			QMPSettings.set( "AllowOnlyOneInstance", page1->allowOnlyOneInstance->isChecked() );
			QMPSettings.set( "Proxy/Use", page1->proxyB->isChecked() && !page1->proxyHostE->text().isEmpty() );
			QMPSettings.set( "Proxy/Host", page1->proxyHostE->text() );
			QMPSettings.set( "Proxy/Port", page1->proxyPortB->value() );
			QMPSettings.set( "Proxy/Login", page1->proxyLoginB->isChecked() && !page1->proxyUserE->text().isEmpty() );
			QMPSettings.set( "Proxy/User", page1->proxyUserE->text() );
			QMPSettings.set( "Proxy/Password", page1->proxyPasswordE->text().toUtf8().toBase64() );
			page1->proxyB->setChecked( QMPSettings.getBool( "Proxy/Use" ) );
			page1->proxyLoginB->setChecked( QMPSettings.getBool( "Proxy/Login" ) );
			( ( QMainWindow * )QMPlay2GUI.mainW )->setTabPosition( Qt::AllDockWidgetAreas, page1->tabsNorths->isChecked() ? QTabWidget::North : QTabWidget::South );
			applyProxy();
		} break;
		case 2:
		{
			QMPSettings.set( "ShortSeek", page2->shortSeekB->value() );
			QMPSettings.set( "LongSeek", page2->longSeekB->value() );
			QMPSettings.set( "AVBufferLocal", page2->bufferLocalB->value() );
			QMPSettings.set( "AVBufferNetwork", page2->bufferNetworkB->value() );
			QMPSettings.set( "PlayIfBuffered", page2->playIfBufferedB->value() );
			QMPSettings.set( "ForceSamplerate", page2->forceSamplerate->isChecked() );
			QMPSettings.set( "Samplerate", page2->samplerateB->value() );
			QMPSettings.set( "MaxVol", page2->maxVolB->value() );
			QMPSettings.set( "ForceChannels", page2->forceChannels->isChecked() );
			QMPSettings.set( "Channels", page2->channelsB->value() );
			QMPSettings.set( "ReplayGain/Enabled", page2->replayGain->isChecked() );
			QMPSettings.set( "ReplayGain/Album", page2->replayGainAlbum->isChecked() );
			QMPSettings.set( "ReplayGain/PreventClipping", page2->replayGainPreventClipping->isChecked() );
			QMPSettings.set( "ReplayGain/Preamp", page2->replayGainPreamp->value() );
			QMPSettings.set( "ShowBufferedTimeOnSlider", page2->showBufferedTimeOnSlider->isChecked() );
			QMPSettings.set( "SavePos", page2->savePos->isChecked() );
			QMPSettings.set( "KeepZoom", page2->keepZoom->isChecked() );
			QMPSettings.set( "KeepARatio", page2->keepARatio->isChecked() );
			QMPSettings.set( "KeepSubtitlesDelay", page2->keepSubtitlesDelay->isChecked() );
			QMPSettings.set( "KeepSubtitlesScale", page2->keepSubtitlesScale->isChecked() );
			QMPSettings.set( "KeepVideoDelay", page2->keepVideoDelay->isChecked() );
			QMPSettings.set( "KeepSpeed", page2->keepSpeed->isChecked() );
			QMPSettings.set( "SyncVtoA", page2->syncVtoA->isChecked() );
			QMPSettings.set( "Silence", page2->silence->isChecked() );
			QMPSettings.set( "ScrollSeek", page2->scrollSeek->isChecked() );
			QMPSettings.set( "RestoreVideoEqualizer", page2->restoreVideoEq->isChecked() );

			QStringList videoWriters, audioWriters, decoders;
			foreach ( QListWidgetItem *wI, page2->modulesList[ 0 ]->findItems( QString(), Qt::MatchContains ) )
				videoWriters += wI->text();
			foreach ( QListWidgetItem *wI, page2->modulesList[ 1 ]->findItems( QString(), Qt::MatchContains ) )
				audioWriters += wI->text();
			foreach ( QListWidgetItem *wI, page2->modulesList[ 2 ]->findItems( QString(), Qt::MatchContains ) )
				decoders += wI->text();
			QMPSettings.set( "videoWriters", videoWriters );
			QMPSettings.set( "audioWriters", audioWriters );
			QMPSettings.set( "decoders", decoders );
			tabCh( 2 );
			tabCh( 1 );

			emit setWheelStep( page2->shortSeekB->value() );
			emit setVolMax( page2->maxVolB->value() );

			SetAudioChannelsMenu();
		} break;
		case 3:
			if ( page3->module && page3->scrollA->widget() )
			{
				( ( Module::SettingsWidget * )page3->scrollA->widget() )->saveSettings();
				page3->module->SetInstances( page3Restart );
				chModule( page3->listW->currentItem() );
			}
			break;
		case 4:
			page4->writeSettings();
			QMPSettings.set( "ApplyToASS/ColorsAndBorders", page4->colorsAndBordersB->isChecked() );
			QMPSettings.set( "ApplyToASS/MarginsAndAlignment", page4->marginsAndAlignmentB->isChecked() );
			QMPSettings.set( "ApplyToASS/FontsAndSpacing", page4->fontsB->isChecked() );
			QMPSettings.set( "ApplyToASS/OverridePlayRes", page4->overridePlayResB->isChecked() );
			QMPSettings.set( "ApplyToASS/ApplyToASS", page4->toASSGB->isChecked() );
			break;
		case 5:
			QMPSettings.set( "OSD/Enabled", page5->enabledB->isChecked() );
			page5->writeSettings();
			break;
		case 6:
			page6->deintSettingsW->writeSettings();
			page6->otherVFiltersW->writeSettings();
			break;
	}
	emit settingsChanged( page, page3Restart );
}
void SettingsWidget::chModule( QListWidgetItem *w )
{
	if ( w )
	{
		page3->module = ( Module * )w->data( Qt::UserRole ).value< void * >();
		QWidget *w = page3->module->getSettingsWidget();
		if ( w )
		{
			QGridLayout *layout = qobject_cast< QGridLayout * >( w->layout() );
			if ( layout )
			{
				layout->setMargin( 2 );
				if ( !layout->property( "NoVHSpacer" ).toBool() )
					AddVHSpacer( *layout );
			}
			page3->scrollA->setWidget( w );
			w->setAutoFillBackground( false );
		}
		else if ( page3->scrollA->widget() )
			page3->scrollA->widget()->close(); //ustawi się na NULL po usunięciu ( WA_DeleteOnClose )
	}
}
void SettingsWidget::tabCh( int idx )
{
	static QString lastM[ 3 ];
	if ( idx == 1 && !page2->modulesList[ 0 ]->count() && !page2->modulesList[ 1 ]->count() && !page2->modulesList[ 2 ]->count() )
	{
		QStringList writers[ 3 ] = { QMPlay2GUI.getModules( "videoWriters", 5 ), QMPlay2GUI.getModules( "audioWriters", 5 ), QMPlay2GUI.getModules( "decoders", 7 ) };
		QVector< QPair< Module *, Module::Info > > pluginsInstances[ 3 ];
		for ( int m = 0 ; m < 3 ; ++m )
			pluginsInstances[ m ].fill( QPair< Module *, Module::Info >(), writers[ m ].size() );
		foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
			foreach ( Module::Info moduleInfo, module->getModulesInfo() )
				for ( int m = 0 ; m < 3 ; ++m )
				{
					const int mIdx = writers[ m ].indexOf( moduleInfo.name );
					if ( mIdx > -1 )
						pluginsInstances[ m ][ mIdx ] = qMakePair( module, moduleInfo );
				}
		for ( int m = 0 ; m < 3 ; ++m )
			for ( int i = 0 ; i < pluginsInstances[ m ].size() ; i++ )
			{
				QListWidgetItem *wI = new QListWidgetItem( writers[ m ][ i ] );
				wI->setData( Qt::UserRole, pluginsInstances[ m ][ i ].first->name() );
				wI->setIcon( QMPlay2GUI.getIcon( pluginsInstances[ m ][ i ].second.img.isNull() ? pluginsInstances[ m ][ i ].first->image() : pluginsInstances[ m ][ i ].second.img ) );
				page2->modulesList[ m ]->addItem( wI );
				if ( writers[ m ][ i ] == lastM[ m ] )
					page2->modulesList[ m ]->setCurrentItem( wI );
			}
		for ( int m = 0 ; m < 3 ; ++m )
			if ( page2->modulesList[ m ]->currentRow() < 0 )
				page2->modulesList[ m ]->setCurrentRow( 0 );
	}
	else if ( idx == 2 )
		for ( int m = 0 ; m < 3 ; ++m )
		{
			QListWidgetItem *currI = page2->modulesList[ m ]->currentItem();
			if ( currI )
				lastM[ m ] = currI->text();
			page2->modulesList[ m ]->clear();
		}
}
void SettingsWidget::openModuleSettings( QListWidgetItem *wI )
{
	QList< QListWidgetItem * > items = page3->listW->findItems( wI->data( Qt::UserRole ).toString(), Qt::MatchExactly );
	if ( items.size() )
	{
		page3->listW->setCurrentItem( items[ 0 ] );
		tabW->setCurrentIndex( 2 );
	}
}
void SettingsWidget::chooseScreenshotDir()
{
	QString dir = QFileDialog::getExistingDirectory( this, tr( "Wybierz katalog" ), page1->screenshotE->text() );
	if ( !dir.isEmpty() )
		page1->screenshotE->setText( dir );
}
void SettingsWidget::page2EnableOrDisable()
{
	page2->samplerateB->setEnabled( page2->forceSamplerate->isChecked() );
	page2->channelsB->setEnabled( page2->forceChannels->isChecked() );
}
void SettingsWidget::setAppearance()
{
	Appearance appearance( this );
	appearance.exec();
}
void SettingsWidget::clearCoversCache()
{
	if ( QMessageBox::question( this, tr( "Potwierdź wyczyszczenie pamięci podręcznej okładek" ), tr( "Czy na pewno chcesz usunąć wszystkie zapamiętane okładki?" ), QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
	{
		QDir dir( QMPlay2Core.getSettingsDir() );
		if ( dir.cd( "Covers" ) )
		{
			foreach ( QString fileName, dir.entryList( QDir::Files ) )
				QFile::remove( dir.absolutePath() + "/" + fileName );
			if ( dir.cdUp() )
				dir.rmdir( "Covers" );
		}
	}
}
void SettingsWidget::resetSettings()
{
	if ( QMessageBox::question( this, tr( "Potwierdź usunięcie ustawień" ), tr( "Czy na pewno chcesz usunąć wszystkie ustawienia?" ), QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
	{
		QMPlay2GUI.removeSettings = true;
		restartApp();
	}
}
