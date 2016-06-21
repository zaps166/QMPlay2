/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <SettingsWidget.hpp>

#include <DeintSettingsW.hpp>
#include <OtherVFiltersW.hpp>
#include <OSDSettingsW.hpp>
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
#include <QDir>

#include <Appearance.hpp>
#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Module.hpp>

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
	#define ICONS_FROM_THEME
#endif
#ifdef Q_OS_WIN
	#include <windows.h>
#endif

class Page1 : public QWidget
{
public:
	QGridLayout *layout;
	QLabel *langL, *styleL, *encodingL, *audioLangL, *subsLangL, *screenshotL;
	QComboBox *langBox, *styleBox, *encodingB, *audioLangB, *subsLangB, *screenshotFormatB;
	QLineEdit *screenshotE;
	QPushButton *setAppearanceB;
#ifdef ICONS_FROM_THEME
	QCheckBox *iconsFromTheme;
#endif
	QCheckBox *showCoversB, *showDirCoversB, *autoOpenVideoWindowB,
#ifdef UPDATER
	*autoUpdatesB,
#endif
	*tabsNorths, *allowOnlyOneInstance, *displayOnlyFileName, *restoreRepeatMode;
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
	QLabel *shortSeekL, *longSeekL, *bufferLocalL, *bufferNetworkL, *backwardBufferNetworkL, *playIfBufferedL, *maxVolL;
	QSpinBox *shortSeekB, *longSeekB, *bufferLocalB, *bufferNetworkB, *samplerateB, *channelsB, *maxVolB;
	QComboBox *backwardBufferNetworkB;
	QDoubleSpinBox *playIfBufferedB;
	QCheckBox *forceSamplerate, *forceChannels, *showBufferedTimeOnSlider, *savePos, *keepZoom, *keepARatio, *syncVtoA, *keepSubtitlesDelay, *keepSubtitlesScale, *keepVideoDelay, *keepSpeed, *silence, *restoreVideoEq, *ignorePlaybackError;

	QGroupBox *replayGain;
	QVBoxLayout *replayGainL;
	QCheckBox *replayGainAlbum, *replayGainPreventClipping;
	QDoubleSpinBox *replayGainPreamp;

	QGroupBox *wheelActionB;
	QVBoxLayout *wheelActionL;
	QRadioButton *wheelSeekB, *wheelVolumeB;

	class ModulesList : public QGroupBox
	{
	public:
		QListWidget *list;
		QWidget *buttonsW;
		QToolButton *moveUp, *moveDown;
		QVBoxLayout *layout1;
		QHBoxLayout *layout2;
	} *modulesList[3];
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
		OSDSettingsW("Subtitles") {}

	QGridLayout *layout2;
	QGroupBox *toASSGB;
	QCheckBox *colorsAndBordersB, *marginsAndAlignmentB, *fontsB, *overridePlayResB;
};
class Page5 : public OSDSettingsW
{
public:
	Page5() :
		OSDSettingsW("OSD") {}

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

static inline void AddVHSpacer(QGridLayout &layout)
{
	layout.addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), layout.rowCount(), 0); //vSpacer
	layout.addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, layout.columnCount()); //hSpacer
}

/**/

void SettingsWidget::InitSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.init("SubtitlesEncoding", "UTF-8");
	QMPSettings.init("AudioLanguage", QString());
	QMPSettings.init("SubtitlesLanguage", QString());
#if QT_VERSION < 0x050000
	QMPSettings.init("screenshotPth", QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
#else
	QMPSettings.init("screenshotPth", QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QDir::homePath()));
#endif
#ifdef Q_OS_WIN
	QMPSettings.init("screenshotFormat", ".bmp");
#else
	QMPSettings.init("screenshotFormat", ".ppm");
#endif
#ifdef ICONS_FROM_THEME
	QMPSettings.init("IconsFromTheme", true);
#else
	QMPSettings.init("IconsFromTheme", false);
#endif
	QMPSettings.init("ShowCovers", true);
	QMPSettings.init("ShowDirCovers", true);
	QMPSettings.init("AutoOpenVideoWindow", true);
#ifdef UPDATER
	if (!QMPSettings.contains("AutoUpdates"))
		QMPSettings.init("AutoUpdates", !QFile::exists(QMPlay2Core.getShareDir() + "noautoupdates"));
#endif
	QMPSettings.init("MainWidget/TabPositionNorth", false);
	QMPSettings.init("AllowOnlyOneInstance", false);
	QMPSettings.init("DisplayOnlyFileName", false);
	QMPSettings.init("RestoreRepeatMode", false);
	QMPSettings.init("Proxy/Use", false);
	QMPSettings.init("Proxy/Host", QString());
	QMPSettings.init("Proxy/Port", 80);
	QMPSettings.init("Proxy/Login", false);
	QMPSettings.init("Proxy/User", QString());
	QMPSettings.init("Proxy/Password", QString());
	QMPSettings.init("ShortSeek", 5);
	QMPSettings.init("LongSeek", 30);
	QMPSettings.init("AVBufferLocal", 75);
	QMPSettings.init("AVBufferNetwork", 25000);
	QMPSettings.init("BackwardBuffer", 1);
	QMPSettings.init("PlayIfBuffered", 1.75);
	QMPSettings.init("MaxVol", 100);
	QMPSettings.init("VolumeL", 100);
	QMPSettings.init("VolumeR", 100);
	QMPSettings.init("ForceSamplerate", false);
	QMPSettings.init("Samplerate", 48000);
	QMPSettings.init("ForceChannels", false);
	QMPSettings.init("Channels", 2);
	QMPSettings.init("ReplayGain/Enabled", false);
	QMPSettings.init("ReplayGain/Album", false);
	QMPSettings.init("ReplayGain/PreventClipping", true);
	QMPSettings.init("ReplayGain/Preamp", 0.0);
	QMPSettings.init("ShowBufferedTimeOnSlider", true);
	QMPSettings.init("WheelAction", true);
	QMPSettings.init("WheelSeek", true);
	QMPSettings.init("SavePos", false);
	QMPSettings.init("KeepZoom", false);
	QMPSettings.init("KeepARatio", false);
	QMPSettings.init("KeepSubtitlesDelay", false);
	QMPSettings.init("KeepSubtitlesScale", false);
	QMPSettings.init("KeepVideoDelay", false);
	QMPSettings.init("KeepSpeed", false);
	QMPSettings.init("SyncVtoA", true);
	QMPSettings.init("Silence", true);
	QMPSettings.init("RestoreVideoEqualizer", false);
	QMPSettings.init("IgnorePlaybackError", false);
	QMPSettings.init("ApplyToASS/ColorsAndBorders", true);
	QMPSettings.init("ApplyToASS/MarginsAndAlignment", false);
	QMPSettings.init("ApplyToASS/FontsAndSpacing", false);
	QMPSettings.init("ApplyToASS/OverridePlayRes", false);
	QMPSettings.init("ApplyToASS/ApplyToASS", false);
	QMPSettings.init("OSD/Enabled", true);
	OSDSettingsW::init("Subtitles", 20, 0, 15, 15, 15, 7, 1.5, 1.5, QColor(0xFF, 0xA8, 0x58, 0xFF), Qt::black, Qt::black);
	OSDSettingsW::init("OSD",       32, 0, 0,  0,  0,  4, 1.5, 1.5, QColor(0xAA, 0xFF, 0x55, 0xFF), Qt::black, Qt::black);
	DeintSettingsW::init();
	applyProxy();
}
void SettingsWidget::SetAudioChannelsMenu()
{
	const bool forceChn = QMPlay2Core.getSettings().getBool("ForceChannels");
	const int chn = QMPlay2Core.getSettings().getInt("Channels");
	bool audioChannelsChecked = false;
	foreach (QAction *act, QMPlay2GUI.menuBar->playback->audioChannels->actions())
		if ((!forceChn && act->objectName() == "auto") || (forceChn && chn == act->objectName().toInt()))
		{
			act->setChecked(true);
			audioChannelsChecked = true;
		}
	if (!audioChannelsChecked)
		QMPlay2GUI.menuBar->playback->audioChannels->other->setChecked(true);
}
void SettingsWidget::SetAudioChannels(int chn)
{
	const bool forceChannels = chn >= 1 && chn <= 8;
	if (forceChannels)
		QMPlay2Core.getSettings().set("Channels", chn);
	QMPlay2Core.getSettings().set("ForceChannels", forceChannels);
}

SettingsWidget::SettingsWidget(int page, const QString &moduleName) :
	wasShow(false),
	moduleIndex(0)
{
	setWindowFlags(Qt::Window);

	Settings &QMPSettings = QMPlay2Core.getSettings();
	int idx, layout_row;

	setWindowTitle(tr("Settings"));
	setAttribute(Qt::WA_DeleteOnClose);

	tabW = new QTabWidget;
	page1 = new Page1;
	page2 = new Page2;
	page3 = new Page3;
	page4 = new Page4;
	page5 = new Page5;
	page6 = new Page6;
	tabW->addTab(page1, tr("General settings"));
	tabW->addTab(page2, tr("Playback settings"));
	tabW->addTab(page3, tr("Modules"));
	tabW->addTab(page4, tr("Subtitles"));
	tabW->addTab(page5, tr("OSD"));
	tabW->addTab(page6, tr("Video filters"));

	applyB = new QPushButton;
	applyB->setText(tr("Apply"));
	connect(applyB, SIGNAL(clicked()), this, SLOT(apply()));

	closeB = new QPushButton;
	closeB->setText(tr("Close"));
	closeB->setShortcut(QKeySequence("Escape"));
	connect(closeB, SIGNAL(clicked()), this, SLOT(close()));

	layout = new QGridLayout(this);
	layout->addWidget(tabW, 0, 0, 1, 3);
	layout->addWidget(applyB, 1, 1, 1, 1);
	layout->addWidget(closeB, 1, 2, 1, 1);
	layout->setMargin(2);

	/* Strona 1 */
	page1->langL = new QLabel;
	page1->langL->setText(tr("Language") + ": ");
	page1->langBox = new QComboBox;
	page1->langBox->addItem("English", "en");
	page1->langBox->setCurrentIndex(0);
	QStringList langs = QMPlay2GUI.getLanguages();
	for (int i = 0; i < langs.count(); i++)
	{
		page1->langBox->addItem(QMPlay2GUI.getLongFromShortLanguage(langs[i]), langs[i]);
		if (QMPlay2Core.getLanguage() == langs[i])
			page1->langBox->setCurrentIndex(i + 1);
	}

	page1->styleL = new QLabel;
	page1->styleL->setText(tr("Style") + ": ");
	page1->styleBox = new QComboBox;
	page1->styleBox->addItems(QStyleFactory::keys());
	idx = page1->styleBox->findText(QApplication::style()->objectName(), Qt::MatchFixedString);
	if (idx > -1 && idx < page1->styleBox->count())
		page1->styleBox->setCurrentIndex(idx);
	connect(page1->styleBox, SIGNAL(currentIndexChanged(int)), this, SLOT(chStyle()));

	page1->encodingL = new QLabel(tr("Subtitles encoding") + ": ");
	page1->encodingB = new QComboBox;
	QStringList encodings;
	foreach (const QByteArray &item, QTextCodec::availableCodecs())
		encodings += QTextCodec::codecForName(item)->name();
	encodings.removeDuplicates();
	page1->encodingB->addItems(encodings);
	idx = page1->encodingB->findText(QMPSettings.getByteArray("SubtitlesEncoding"));
	if (idx > -1)
		page1->encodingB->setCurrentIndex(idx);

	const QString audioLang = QMPSettings.getString("AudioLanguage");
	const QString subsLang = QMPSettings.getString("SubtitlesLanguage");
	page1->audioLangL = new QLabel(tr("Default audio language") + ": ");
	page1->subsLangL = new QLabel(tr("Default subtitles language") + ": ");
	page1->audioLangB = new QComboBox;
	page1->subsLangB = new QComboBox;
	page1->audioLangB->addItem(tr("Default or first stream"));
	page1->subsLangB->addItem(tr("Default or first stream"));
	foreach (const QString &lang, QMPlay2Core.getLanguagesMap())
	{
		page1->audioLangB->addItem(lang);
		page1->subsLangB->addItem(lang);
		if (lang == audioLang)
			page1->audioLangB->setCurrentIndex(page1->audioLangB->count() - 1);
		if (lang == subsLang)
			page1->subsLangB->setCurrentIndex(page1->subsLangB->count() - 1);
	}

	page1->screenshotL = new QLabel(tr("Screenshots path") + ": ");
	page1->screenshotE = new QLineEdit;
	page1->screenshotE->setReadOnly(true);
	page1->screenshotE->setText(QMPSettings.getString("screenshotPth"));
	page1->screenshotFormatB = new QComboBox;
	page1->screenshotFormatB->addItems(QStringList() << ".ppm" << ".bmp" << ".png");
	page1->screenshotFormatB->setCurrentIndex(page1->screenshotFormatB->findText(QMPSettings.getString("screenshotFormat")));
	page1->screenshotB = new QToolButton;
	page1->screenshotB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
	page1->screenshotB->setToolTip(tr("Browse"));
	connect(page1->screenshotB, SIGNAL(clicked()), this, SLOT(chooseScreenshotDir()));

	page1->setAppearanceB = new QPushButton(tr("Set appearance"));
	connect(page1->setAppearanceB, SIGNAL(clicked()), this, SLOT(setAppearance()));

#ifdef ICONS_FROM_THEME
	page1->iconsFromTheme = new QCheckBox(tr("Use system icon set"));
	page1->iconsFromTheme->setChecked(QMPSettings.getBool("IconsFromTheme"));
#endif

	page1->showCoversB = new QCheckBox(tr("Show covers"));
	page1->showCoversB->setChecked(QMPSettings.getBool("ShowCovers"));

	page1->showDirCoversB = new QCheckBox(tr("Show covers from directory if there aren't in the music file"));
	page1->showDirCoversB->setChecked(QMPSettings.getBool("ShowDirCovers"));
	connect(page1->showCoversB, SIGNAL(clicked(bool)), page1->showDirCoversB, SLOT(setEnabled(bool)));
	page1->showDirCoversB->setEnabled(page1->showCoversB->isChecked());

	page1->autoOpenVideoWindowB = new QCheckBox(tr("Automatically opening video window"));
	page1->autoOpenVideoWindowB->setChecked(QMPSettings.getBool("AutoOpenVideoWindow"));

#ifdef UPDATER
	page1->autoUpdatesB = new QCheckBox(tr("Automatically check and download updates"));
	page1->autoUpdatesB->setChecked(QMPSettings.getBool("AutoUpdates"));
#endif

	page1->tabsNorths = new QCheckBox(tr("Show tabs at the top of the main window"));
	page1->tabsNorths->setChecked(QMPSettings.getBool("MainWidget/TabPositionNorth"));

	page1->allowOnlyOneInstance = new QCheckBox(tr("Allow only one instance"));
	page1->allowOnlyOneInstance->setChecked(QMPSettings.getBool("AllowOnlyOneInstance"));

	page1->displayOnlyFileName = new QCheckBox(tr("Always display only file names in playlist"));
	page1->displayOnlyFileName->setChecked(QMPSettings.getBool("DisplayOnlyFileName"));

	page1->restoreRepeatMode = new QCheckBox(tr("Remember repeat mode"));
	page1->restoreRepeatMode->setChecked(QMPSettings.getBool("RestoreRepeatMode"));


	page1->proxyB = new QGroupBox(tr("Use proxy server"));
	page1->proxyB->setCheckable(true);
	page1->proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));

	page1->proxyHostE = new QLineEdit(QMPSettings.getString("Proxy/Host"));
	page1->proxyHostE->setPlaceholderText(tr("Proxy server address"));

	page1->proxyPortB = new QSpinBox;
	page1->proxyPortB->setRange(1, 65535);
	page1->proxyPortB->setValue(QMPSettings.getInt("Proxy/Port"));
	page1->proxyPortB->setToolTip(tr("Proxy server port"));

	page1->proxyLoginB = new QGroupBox(tr("Proxy server needs login"));
	page1->proxyLoginB->setCheckable(true);
	page1->proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));

	page1->proxyUserE = new QLineEdit(QMPSettings.getString("Proxy/User"));
	page1->proxyUserE->setPlaceholderText(tr("User name"));

	page1->proxyPasswordE = new QLineEdit(QByteArray::fromBase64(QMPSettings.getByteArray("Proxy/Password")));
	page1->proxyPasswordE->setEchoMode(QLineEdit::Password);
	page1->proxyPasswordE->setPlaceholderText(tr("Password"));

	page1->proxyLoginL = new QVBoxLayout(page1->proxyLoginB);
	page1->proxyLoginL->addWidget(page1->proxyUserE);
	page1->proxyLoginL->addWidget(page1->proxyPasswordE);

	page1->proxyL = new QGridLayout(page1->proxyB);
	page1->proxyL->addWidget(page1->proxyLoginB, 0, 0, 1, 2);
	page1->proxyL->addWidget(page1->proxyHostE, 1, 0, 1, 1);
	page1->proxyL->addWidget(page1->proxyPortB, 1, 1, 1, 1);


	page1->clearCoversCache = new QPushButton;
	page1->clearCoversCache->setText(tr("Clear covers cache"));
	page1->clearCoversCache->setIcon(QMPlay2Core.getIconFromTheme("view-refresh"));
	connect(page1->clearCoversCache, SIGNAL(clicked()), this, SLOT(clearCoversCache()));

	page1->resetSettingsB = new QPushButton;
	page1->resetSettingsB->setText(tr("Reset settings"));
	page1->resetSettingsB->setIcon(QMPlay2Core.getIconFromTheme("view-refresh"));
	connect(page1->resetSettingsB, SIGNAL(clicked()), this, SLOT(resetSettings()));

	layout_row = 0;
	page1->layout = new QGridLayout(page1);
	page1->layout->setMargin(3);
	page1->layout->setSpacing(1);
	page1->layout->addWidget(page1->langL, layout_row, 0, 1, 1);
	page1->layout->addWidget(page1->langBox, layout_row++, 1, 1, 3);
	page1->layout->addWidget(page1->styleL, layout_row, 0, 1, 1);
	page1->layout->addWidget(page1->styleBox, layout_row++, 1, 1, 3);
	page1->layout->addWidget(page1->encodingL, layout_row, 0, 1, 1);
	page1->layout->addWidget(page1->encodingB, layout_row++, 1, 1, 3);
	page1->layout->addWidget(page1->audioLangL, layout_row, 0, 1, 1);
	page1->layout->addWidget(page1->audioLangB, layout_row++, 1, 1, 3);
	page1->layout->addWidget(page1->subsLangL, layout_row, 0, 1, 1);
	page1->layout->addWidget(page1->subsLangB, layout_row++, 1, 1, 3);
	page1->layout->addWidget(page1->screenshotL, layout_row, 0, 1, 1);
	page1->layout->addWidget(page1->screenshotE, layout_row, 1, 1, 1);
	page1->layout->addWidget(page1->screenshotFormatB, layout_row, 2, 1, 1);
	page1->layout->addWidget(page1->screenshotB, layout_row++, 3, 1, 1);
	page1->layout->addWidget(page1->setAppearanceB, layout_row, 1, 1, 3);
#ifdef ICONS_FROM_THEME
	page1->layout->addWidget(page1->iconsFromTheme, layout_row, 0, 1, 1);
#endif
	layout_row++;
	page1->layout->addWidget(page1->showCoversB, layout_row++, 0, 1, 4);
	page1->layout->addWidget(page1->showDirCoversB, layout_row++, 0, 1, 4);
	page1->layout->addWidget(page1->autoOpenVideoWindowB, layout_row++, 0, 1, 4);
#ifdef UPDATER
	page1->layout->addWidget(page1->autoUpdatesB, layout_row++, 0, 1, 4);
#endif
	page1->layout->addWidget(page1->tabsNorths, layout_row++, 0, 1, 4);
	page1->layout->addWidget(page1->allowOnlyOneInstance, layout_row++, 0, 1, 4);
	page1->layout->addWidget(page1->displayOnlyFileName, layout_row++, 0, 1, 4);
	page1->layout->addWidget(page1->restoreRepeatMode, layout_row++, 0, 1, 4);
	page1->layout->addWidget(page1->proxyB, layout_row++, 0, 1, 4);
	page1->layout->addWidget(page1->clearCoversCache, layout_row, 0, 1, 1);
	page1->layout->addWidget(page1->resetSettingsB, layout_row++, 1, 1, 3);
	page1->layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), page1->layout->rowCount(), 0); //vSpacer

	/* Strona 2 */
	page2->shortSeekL = new QLabel(tr("Short seeking (left and right arrows)") + ": ");
	page2->shortSeekB = new QSpinBox;
	page2->shortSeekB->setRange(2, 20);
	page2->shortSeekB->setSuffix(" " + tr("sec"));
	page2->shortSeekB->setValue(QMPSettings.getInt("ShortSeek"));

	page2->longSeekL = new QLabel(tr("Long seeking (up and down arrows)") + ": ");
	page2->longSeekB = new QSpinBox;
	page2->longSeekB->setRange(30, 300);
	page2->longSeekB->setSuffix(" " + tr("sec"));
	page2->longSeekB->setValue(QMPSettings.getInt("LongSeek"));

	page2->bufferLocalL = new QLabel(tr("Local buffer size (A/V packages count)") + ": ");
	page2->bufferLocalB = new QSpinBox;
	page2->bufferLocalB->setRange(1, 1000);
	page2->bufferLocalB->setValue(QMPSettings.getInt("AVBufferLocal"));

	page2->bufferNetworkL = new QLabel(tr("Network buffer size (A/V packages count)") + ": ");
	page2->bufferNetworkB = new QSpinBox;
	page2->bufferNetworkB->setRange(100, 1000000);
	page2->bufferNetworkB->setValue(QMPSettings.getInt("AVBufferNetwork"));

	page2->backwardBufferNetworkL = new QLabel(tr("Percent of packages for backwards rewinding") + ": ");
	page2->backwardBufferNetworkB = new QComboBox;
	page2->backwardBufferNetworkB->addItems(QStringList() << "10%" << "25%" << "50%");
	page2->backwardBufferNetworkB->setCurrentIndex(QMPSettings.getUInt("BackwardBuffer"));

	page2->playIfBufferedL = new QLabel(tr("Start playback internet stream if it is buffered") + ": ");
	page2->playIfBufferedB = new QDoubleSpinBox;
	page2->playIfBufferedB->setRange(0.0, 5.0);
	page2->playIfBufferedB->setSingleStep(0.25);
	page2->playIfBufferedB->setSuffix(" " + tr("sec"));
	page2->playIfBufferedB->setValue(QMPSettings.getDouble("PlayIfBuffered"));

	page2->maxVolL = new QLabel(tr("Maximum volume"));
	page2->maxVolB = new QSpinBox;
	page2->maxVolB->setRange(100, 1000);
	page2->maxVolB->setSingleStep(50);
	page2->maxVolB->setSuffix(" %");
	page2->maxVolB->setValue(QMPSettings.getInt("MaxVol"));

	page2->forceSamplerate = new QCheckBox(tr("Force samplerate"));
	page2->forceSamplerate->setChecked(QMPSettings.getBool("ForceSamplerate"));
	connect(page2->forceSamplerate, SIGNAL(stateChanged(int)), this, SLOT(page2EnableOrDisable()));
	page2->samplerateB = new QSpinBox;
	page2->samplerateB->setRange(4000, 192000);
	page2->samplerateB->setValue(QMPSettings.getInt("Samplerate"));

	page2->forceChannels = new QCheckBox(tr("Force channels conversion"));
	page2->forceChannels->setChecked(QMPSettings.getBool("ForceChannels"));
	connect(page2->forceChannels, SIGNAL(stateChanged(int)), this, SLOT(page2EnableOrDisable()));
	page2->channelsB = new QSpinBox;
	page2->channelsB->setRange(1, 8);
	page2->channelsB->setValue(QMPSettings.getInt("Channels"));

	page2EnableOrDisable();


	page2->replayGain = new QGroupBox(tr("Use replay gain if available"));
	page2->replayGain->setCheckable(true);
	page2->replayGain->setChecked(QMPSettings.getBool("ReplayGain/Enabled"));

	page2->replayGainL = new QVBoxLayout(page2->replayGain);

	page2->replayGainAlbum = new QCheckBox(tr("Album mode for replay gain"));
	page2->replayGainAlbum->setChecked(QMPSettings.getBool("ReplayGain/Album"));

	page2->replayGainPreventClipping = new QCheckBox(tr("Prevent clipping"));
	page2->replayGainPreventClipping->setChecked(QMPSettings.getBool("ReplayGain/PreventClipping"));

	page2->replayGainPreamp = new QDoubleSpinBox;
	page2->replayGainPreamp->setRange(-15.0, 15.0);
	page2->replayGainPreamp->setPrefix(tr("Amplify") + ": ");
	page2->replayGainPreamp->setSuffix(" dB");
	page2->replayGainPreamp->setValue(QMPSettings.getDouble("ReplayGain/Preamp"));

	page2->replayGainL->addWidget(page2->replayGainAlbum);
	page2->replayGainL->addWidget(page2->replayGainPreventClipping);
	page2->replayGainL->addWidget(page2->replayGainPreamp);


	page2->wheelActionB = new QGroupBox(tr("Mouse wheel action on video dock"));
	page2->wheelActionB->setCheckable(true);
	page2->wheelActionB->setChecked(QMPSettings.getBool("WheelAction"));

	page2->wheelActionL = new QVBoxLayout(page2->wheelActionB);

	page2->wheelSeekB = new QRadioButton(tr("Mouse wheel scrolls music/movie"));
	page2->wheelSeekB->setChecked(QMPSettings.getBool("WheelSeek"));

	page2->wheelVolumeB = new QRadioButton(tr("Mouse wheel changes the volume"));
	page2->wheelVolumeB->setChecked(QMPSettings.getBool("WheelVolume"));

	page2->wheelActionL->addWidget(page2->wheelSeekB);
	page2->wheelActionL->addWidget(page2->wheelVolumeB);


	page2->showBufferedTimeOnSlider = new QCheckBox(tr("Show buffered data indicator on slider"));
	page2->showBufferedTimeOnSlider->setChecked(QMPSettings.getBool("ShowBufferedTimeOnSlider"));

	page2->savePos = new QCheckBox(tr("Remember playback position"));
	page2->savePos->setChecked(QMPSettings.getBool("SavePos"));

	page2->keepZoom = new QCheckBox(tr("Keep zoom"));;
	page2->keepZoom->setChecked(QMPSettings.getBool("KeepZoom"));

	page2->keepARatio = new QCheckBox(tr("Keep aspect ratio"));
	page2->keepARatio->setChecked(QMPSettings.getBool("KeepARatio"));

	page2->keepSubtitlesDelay = new QCheckBox(tr("Keep subtitles delay"));
	page2->keepSubtitlesDelay->setChecked(QMPSettings.getBool("KeepSubtitlesDelay"));

	page2->keepSubtitlesScale = new QCheckBox(tr("Keep subtitles scale"));
	page2->keepSubtitlesScale->setChecked(QMPSettings.getBool("KeepSubtitlesScale"));

	page2->keepVideoDelay = new QCheckBox(tr("Keep video delay"));
	page2->keepVideoDelay->setChecked(QMPSettings.getBool("KeepVideoDelay"));

	page2->keepSpeed = new QCheckBox(tr("Keep speed"));
	page2->keepSpeed->setChecked(QMPSettings.getBool("KeepSpeed"));

	page2->syncVtoA = new QCheckBox(tr("Video to audio sync (frame skipping)"));
	page2->syncVtoA->setChecked(QMPSettings.getBool("SyncVtoA"));

	page2->silence = new QCheckBox(tr("Fade sound"));
	page2->silence->setChecked(QMPSettings.getBool("Silence"));

	page2->restoreVideoEq = new QCheckBox(tr("Remember video equalizer settings"));
	page2->restoreVideoEq->setChecked(QMPSettings.getBool("RestoreVideoEqualizer"));

	page2->ignorePlaybackError = new QCheckBox(tr("Play next entry after playback error"));
	page2->ignorePlaybackError->setChecked(QMPSettings.getBool("IgnorePlaybackError"));

	for (int m = 0; m < 3; ++m)
	{
		Page2::ModulesList *&mL = page2->modulesList[m];
		mL = new Page2::ModulesList;

		mL->list = new QListWidget;
		mL->list->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
		connect(mL->list, SIGNAL(itemDoubleClicked (QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
		mL->list->setSelectionMode(QAbstractItemView::ExtendedSelection);
		mL->list->setDragDropMode(QAbstractItemView::InternalMove);

		mL->buttonsW = new QWidget;

		mL->moveUp = new QToolButton;
		mL->moveUp->setArrowType(Qt::UpArrow);
		mL->moveUp->setToolTip(tr("Move up"));
		connect(mL->moveUp, SIGNAL(clicked()), this, SLOT(moveModule()));
		mL->moveUp->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));

		mL->moveDown = new QToolButton;
		mL->moveDown->setArrowType(Qt::DownArrow);
		mL->moveDown->setToolTip(tr("Move down"));
		connect(mL->moveDown, SIGNAL(clicked()), this, SLOT(moveModule()));
		mL->moveDown->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));

		mL->layout1 = new QVBoxLayout(mL->buttonsW);
		mL->layout1->addWidget(mL->moveUp);
		mL->layout1->addWidget(mL->moveDown);
		mL->layout1->setSpacing(2);
		mL->layout1->setMargin(0);

		mL->layout2 = new QHBoxLayout(mL);
		mL->layout2->addWidget(mL->list);
		mL->layout2->addWidget(mL->buttonsW);
		mL->layout2->setSpacing(2);
		mL->layout2->setMargin(0);
	}
	page2->modulesList[0]->setTitle(tr("Video output priority"));
	page2->modulesList[1]->setTitle(tr("Audio output priority"));
	page2->modulesList[2]->setTitle(tr("Decoders priority"));

	layout_row = 0;
	page2->w = new QWidget;
	page2->layout2 = new QGridLayout(page2->w);
	page2->layout2->addWidget(page2->shortSeekL, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->shortSeekB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->longSeekL, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->longSeekB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->bufferLocalL, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->bufferLocalB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->bufferNetworkL, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->bufferNetworkB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->backwardBufferNetworkL, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->backwardBufferNetworkB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->playIfBufferedL, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->playIfBufferedB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->maxVolL, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->maxVolB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->forceSamplerate, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->samplerateB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->forceChannels, layout_row, 0, 1, 1);
	page2->layout2->addWidget(page2->channelsB, layout_row++, 1, 1, 1);
	page2->layout2->addWidget(page2->replayGain, layout_row++, 0, 1, 2);
	page2->layout2->addWidget(page2->wheelActionB, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->showBufferedTimeOnSlider, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->savePos, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->keepZoom, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->keepARatio, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->keepSubtitlesDelay, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->keepSubtitlesScale, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->keepVideoDelay, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->keepSpeed, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->syncVtoA, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->silence, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->restoreVideoEq, layout_row++, 0, 1, 3);
	page2->layout2->addWidget(page2->ignorePlaybackError, layout_row++, 0, 1, 3);
	page2->layout2->setMargin(0);
	page2->layout2->setSpacing(2);

	page2->scrollA = new QScrollArea;
	page2->scrollA->setFrameShape(QFrame::NoFrame);
	page2->scrollA->setWidget(page2->w);

	page2->layout1 = new QGridLayout(page2);
	page2->layout1->setMargin(2);
	page2->layout1->setSpacing(3);
	page2->layout1->addWidget(page2->scrollA, 0, 0, 1, 3);
	for (int m = 0; m < 3; ++m)
		page2->layout1->addWidget(page2->modulesList[m], 1, m);

	/* Strona 3 */
	page3->module = NULL;

	page3->listW = new QListWidget;
	page3->listW->setIconSize(QSize(32, 32));
	page3->listW->setMinimumSize(200, 0);
	page3->listW->setMaximumSize(200, 16777215);
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
	{
		QListWidgetItem *tWI = new QListWidgetItem(module->name());
		tWI->setData(Qt::UserRole, qVariantFromValue((void *)module));
		QString toolTip = tr("Contains") + ":";
		foreach (Module::Info mod, module->getModulesInfo(true))
		{
			toolTip += "<p>&nbsp;&nbsp;&nbsp;&nbsp;";
			if (!mod.imgPath().isEmpty())
				toolTip += "<img width='22' height='22' src='" + mod.imgPath() + "'/> ";
			else
				toolTip += "- ";
			toolTip += mod.name + "</p>";
		}
		tWI->setToolTip("<html>" + toolTip + "</html>");
		tWI->setIcon(QMPlay2GUI.getIcon(module->image()));
		page3->listW->addItem(tWI);
		if (page == 2 && !moduleName.isEmpty() && module->name() == moduleName)
			moduleIndex = page3->listW->count() - 1;
	}

	page3->scrollA = new QScrollArea;
	page3->scrollA->setWidgetResizable(true);
	page3->scrollA->setFrameShape(QFrame::NoFrame);

	page3->layout = new QHBoxLayout(page3);
	page3->layout->setMargin(1);
	page3->layout->setSpacing(1);
	page3->layout->addWidget(page3->listW);
	page3->layout->addWidget(page3->scrollA);
	connect(page3->listW, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(chModule(QListWidgetItem *)));

	/* Strona 4 */
	page4->colorsAndBordersB = new QCheckBox(tr("Colors and borders"));
	page4->colorsAndBordersB->setChecked(QMPSettings.getBool("ApplyToASS/ColorsAndBorders"));

	page4->marginsAndAlignmentB = new QCheckBox(tr("Margins and alignment"));
	page4->marginsAndAlignmentB->setChecked(QMPSettings.getBool("ApplyToASS/MarginsAndAlignment"));

	page4->fontsB = new QCheckBox(tr("Fonts and spacing"));
	page4->fontsB->setChecked(QMPSettings.getBool("ApplyToASS/FontsAndSpacing"));

	page4->overridePlayResB = new QCheckBox(tr("Use the same size"));
	page4->overridePlayResB->setChecked(QMPSettings.getBool("ApplyToASS/OverridePlayRes"));

	page4->toASSGB = new QGroupBox(tr("Apply for ASS/SSA subtitles"));
	page4->toASSGB->setCheckable(true);
	page4->toASSGB->setChecked(QMPSettings.getBool("ApplyToASS/ApplyToASS"));

	page4->layout2 = new QGridLayout(page4->toASSGB);
	page4->layout2->addWidget(page4->colorsAndBordersB, 0, 0, 1, 1);
	page4->layout2->addWidget(page4->marginsAndAlignmentB, 1, 0, 1, 1);
	page4->layout2->addWidget(page4->fontsB, 0, 1, 1, 1);
	page4->layout2->addWidget(page4->overridePlayResB, 1, 1, 1, 1);

	page4->layout->addWidget(page4->toASSGB, 2, 0, 1, 5);
	AddVHSpacer(*page4->layout);

	/* Strona 5 */
	page5->enabledB = new QCheckBox(tr("OSD enabled"));
	page5->enabledB->setChecked(QMPSettings.getBool("OSD/Enabled"));

	page5->layout->addWidget(page5->enabledB, 2, 0, 1, 5);
	AddVHSpacer(*page5->layout);

	/* Strona 6 */
	page6->deintSettingsW = new DeintSettingsW;

	page6->otherVFiltersL = new QLabel(tr("Software video filters") + ":");
	page6->otherVFiltersW = new OtherVFiltersW;
	connect(page6->otherVFiltersW, SIGNAL(itemDoubleClicked (QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));

	page6->layout = new QGridLayout(page6);
	page6->layout->setMargin(1);
	page6->layout->addWidget(page6->deintSettingsW, 0, 0, 1, 1);
	page6->layout->addWidget(page6->otherVFiltersL, 1, 0, 1, 1);
	page6->layout->addWidget(page6->otherVFiltersW, 2, 0, 1, 1);


	connect(tabW, SIGNAL(currentChanged(int)), this, SLOT(tabCh(int)));
	tabW->setCurrentIndex(page);

	show();
}

void SettingsWidget::setAudioChannels()
{
	page2->forceChannels->setChecked(QMPlay2Core.getSettings().getBool("ForceChannels"));
	page2->channelsB->setValue(QMPlay2Core.getSettings().getInt("Channels"));
}

void SettingsWidget::applyProxy()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QNetworkProxy proxy;
	if (!QMPSettings.getBool("Proxy/Use"))
	{
#ifdef Q_OS_WIN
		SetEnvironmentVariable("http_proxy", NULL);
#else
		unsetenv("http_proxy");
#endif
	}
	else
	{
		proxy.setType(QNetworkProxy::Socks5Proxy);
		proxy.setHostName(QMPSettings.getString("Proxy/Host"));
		proxy.setPort(QMPSettings.getInt("Proxy/Port"));
		if (QMPSettings.getBool("Proxy/Login"))
		{
			proxy.setUser(QMPSettings.getString("Proxy/User"));
			proxy.setPassword(QByteArray::fromBase64(QMPSettings.getString("Proxy/Password").toLatin1()));
		}

		/* Proxy env for FFmpeg */
		QString proxyEnv = QString("http://" + proxy.hostName() + ':' + QString::number(proxy.port()));
		if (!proxy.user().isEmpty())
		{
			QString auth = proxy.user();
			if (!proxy.password().isEmpty())
				auth += ':' + proxy.password();
			auth += '@';
			proxyEnv.insert(7, auth);
		}
#ifdef Q_OS_WIN
		SetEnvironmentVariable("http_proxy", proxyEnv.toLocal8Bit());
#else
		setenv("http_proxy", proxyEnv.toLocal8Bit(), true);
#endif
	}
	QNetworkProxy::setApplicationProxy(proxy);
}

void SettingsWidget::restartApp()
{
	QMPlay2GUI.restartApp = true;
	close();
	QMPlay2GUI.mainW->close();
}

void SettingsWidget::showEvent(QShowEvent *)
{
	if (!wasShow)
	{
		QMPlay2GUI.restoreGeometry("SettingsWidget/Geometry", this, QSize(630, 425));
		page3->listW->setCurrentRow(moduleIndex);
		wasShow = true;
	}
}
void SettingsWidget::closeEvent(QCloseEvent *)
{
	QMPlay2Core.getSettings().set("SettingsWidget/Geometry", geometry());
}

void SettingsWidget::chStyle()
{
	QString newStyle = page1->styleBox->currentText().toLower();
	if (QApplication::style()->objectName() != newStyle)
	{
		QMPlay2Core.getSettings().set("Style", newStyle);
		QMPlay2GUI.setStyle();
	}
}
void SettingsWidget::apply()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	bool page3Restart = false;
	const int page = tabW->currentIndex() + 1;
	switch (page)
	{
		case 1:
		{
			if (QMPlay2Core.getLanguage() != page1->langBox->itemData(page1->langBox->currentIndex()).toString())
			{
				QMPSettings.set("Language", page1->langBox->itemData(page1->langBox->currentIndex()).toString());
				QMPlay2GUI.setLanguage();
				QMessageBox::information(this, tr("New language"), tr("To set up a new language, the program will start again!"));
				restartApp();
			}
#ifdef ICONS_FROM_THEME
			if (page1->iconsFromTheme->isChecked() != QMPSettings.getBool("IconsFromTheme"))
			{
				QMPSettings.set("IconsFromTheme", page1->iconsFromTheme->isChecked());
				if (!QMPlay2GUI.restartApp)
				{
					QMessageBox::information(this, tr("Changing icons"), tr("To apply the icons change program will start again!"));
					restartApp();
				}
			}
#endif
			QMPSettings.set("SubtitlesEncoding", page1->encodingB->currentText().toUtf8());
			QMPSettings.set("AudioLanguage", page1->audioLangB->currentIndex() > 0 ? page1->audioLangB->currentText() : QString());
			QMPSettings.set("SubtitlesLanguage", page1->subsLangB->currentIndex() > 0 ? page1->subsLangB->currentText() : QString());
			QMPSettings.set("screenshotPth", page1->screenshotE->text());
			QMPSettings.set("screenshotFormat", page1->screenshotFormatB->currentText());
			QMPSettings.set("ShowCovers", page1->showCoversB->isChecked());
			QMPSettings.set("ShowDirCovers", page1->showDirCoversB->isChecked());
			QMPSettings.set("AutoOpenVideoWindow", page1->autoOpenVideoWindowB->isChecked());
#ifdef UPDATER
			QMPSettings.set("AutoUpdates", page1->autoUpdatesB->isChecked());
#endif
			QMPSettings.set("MainWidget/TabPositionNorth", page1->tabsNorths->isChecked());
			QMPSettings.set("AllowOnlyOneInstance", page1->allowOnlyOneInstance->isChecked());
			QMPSettings.set("DisplayOnlyFileName", page1->displayOnlyFileName->isChecked());
			QMPSettings.set("RestoreRepeatMode", page1->restoreRepeatMode->isChecked());
			QMPSettings.set("Proxy/Use", page1->proxyB->isChecked() && !page1->proxyHostE->text().isEmpty());
			QMPSettings.set("Proxy/Host", page1->proxyHostE->text());
			QMPSettings.set("Proxy/Port", page1->proxyPortB->value());
			QMPSettings.set("Proxy/Login", page1->proxyLoginB->isChecked() && !page1->proxyUserE->text().isEmpty());
			QMPSettings.set("Proxy/User", page1->proxyUserE->text());
			QMPSettings.set("Proxy/Password", page1->proxyPasswordE->text().toUtf8().toBase64());
			page1->proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));
			page1->proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));
			((QMainWindow *)QMPlay2GUI.mainW)->setTabPosition(Qt::AllDockWidgetAreas, page1->tabsNorths->isChecked() ? QTabWidget::North : QTabWidget::South);
			applyProxy();
		} break;
		case 2:
		{
			QMPSettings.set("ShortSeek", page2->shortSeekB->value());
			QMPSettings.set("LongSeek", page2->longSeekB->value());
			QMPSettings.set("AVBufferLocal", page2->bufferLocalB->value());
			QMPSettings.set("AVBufferNetwork", page2->bufferNetworkB->value());
			QMPSettings.set("BackwardBuffer", page2->backwardBufferNetworkB->currentIndex());
			QMPSettings.set("PlayIfBuffered", page2->playIfBufferedB->value());
			QMPSettings.set("ForceSamplerate", page2->forceSamplerate->isChecked());
			QMPSettings.set("Samplerate", page2->samplerateB->value());
			QMPSettings.set("MaxVol", page2->maxVolB->value());
			QMPSettings.set("ForceChannels", page2->forceChannels->isChecked());
			QMPSettings.set("Channels", page2->channelsB->value());
			QMPSettings.set("ReplayGain/Enabled", page2->replayGain->isChecked());
			QMPSettings.set("ReplayGain/Album", page2->replayGainAlbum->isChecked());
			QMPSettings.set("ReplayGain/PreventClipping", page2->replayGainPreventClipping->isChecked());
			QMPSettings.set("ReplayGain/Preamp", page2->replayGainPreamp->value());
			QMPSettings.set("WheelAction", page2->wheelActionB->isChecked());
			QMPSettings.set("WheelSeek", page2->wheelSeekB->isChecked());
			QMPSettings.set("WheelVolume", page2->wheelVolumeB->isChecked());
			QMPSettings.set("ShowBufferedTimeOnSlider", page2->showBufferedTimeOnSlider->isChecked());
			QMPSettings.set("SavePos", page2->savePos->isChecked());
			QMPSettings.set("KeepZoom", page2->keepZoom->isChecked());
			QMPSettings.set("KeepARatio", page2->keepARatio->isChecked());
			QMPSettings.set("KeepSubtitlesDelay", page2->keepSubtitlesDelay->isChecked());
			QMPSettings.set("KeepSubtitlesScale", page2->keepSubtitlesScale->isChecked());
			QMPSettings.set("KeepVideoDelay", page2->keepVideoDelay->isChecked());
			QMPSettings.set("KeepSpeed", page2->keepSpeed->isChecked());
			QMPSettings.set("SyncVtoA", page2->syncVtoA->isChecked());
			QMPSettings.set("Silence", page2->silence->isChecked());
			QMPSettings.set("RestoreVideoEqualizer", page2->restoreVideoEq->isChecked());
			QMPSettings.set("IgnorePlaybackError", page2->ignorePlaybackError->isChecked());

			QStringList videoWriters, audioWriters, decoders;
			foreach (QListWidgetItem *wI, page2->modulesList[0]->list->findItems(QString(), Qt::MatchContains))
				videoWriters += wI->text();
			foreach (QListWidgetItem *wI, page2->modulesList[1]->list->findItems(QString(), Qt::MatchContains))
				audioWriters += wI->text();
			foreach (QListWidgetItem *wI, page2->modulesList[2]->list->findItems(QString(), Qt::MatchContains))
				decoders += wI->text();
			QMPSettings.set("videoWriters", videoWriters);
			QMPSettings.set("audioWriters", audioWriters);
			QMPSettings.set("decoders", decoders);
			tabCh(2);
			tabCh(1);

			emit setWheelStep(page2->shortSeekB->value());
			emit setVolMax(page2->maxVolB->value());

			SetAudioChannelsMenu();
		} break;
		case 3:
			if (page3->module && page3->scrollA->widget())
			{
				Module::SettingsWidget *settingsWidget = (Module::SettingsWidget *)page3->scrollA->widget();
				settingsWidget->saveSettings();
				settingsWidget->flushSettings();
				page3->module->setInstances(page3Restart);
				chModule(page3->listW->currentItem());
			}
			break;
		case 4:
			page4->writeSettings();
			QMPSettings.set("ApplyToASS/ColorsAndBorders", page4->colorsAndBordersB->isChecked());
			QMPSettings.set("ApplyToASS/MarginsAndAlignment", page4->marginsAndAlignmentB->isChecked());
			QMPSettings.set("ApplyToASS/FontsAndSpacing", page4->fontsB->isChecked());
			QMPSettings.set("ApplyToASS/OverridePlayRes", page4->overridePlayResB->isChecked());
			QMPSettings.set("ApplyToASS/ApplyToASS", page4->toASSGB->isChecked());
			break;
		case 5:
			QMPSettings.set("OSD/Enabled", page5->enabledB->isChecked());
			page5->writeSettings();
			break;
		case 6:
			page6->deintSettingsW->writeSettings();
			page6->otherVFiltersW->writeSettings();
			break;
	}
	if (page != 3)
		QMPSettings.flush();
	emit settingsChanged(page, page3Restart);
}
void SettingsWidget::chModule(QListWidgetItem *w)
{
	if (w)
	{
		page3->module = (Module *)w->data(Qt::UserRole).value<void *>();
		QWidget *w = page3->module->getSettingsWidget();
		if (w)
		{
			QGridLayout *layout = qobject_cast<QGridLayout *>(w->layout());
			if (layout)
			{
				layout->setMargin(2);
				if (!layout->property("NoVHSpacer").toBool())
					AddVHSpacer(*layout);
			}
			page3->scrollA->setWidget(w);
			w->setAutoFillBackground(false);
		}
		else if (page3->scrollA->widget())
			page3->scrollA->widget()->close(); //ustawi się na NULL po usunięciu (WA_DeleteOnClose)
	}
}
void SettingsWidget::tabCh(int idx)
{
	if (idx == 1 && !page2->modulesList[0]->list->count() && !page2->modulesList[1]->list->count() && !page2->modulesList[2]->list->count())
	{
		QStringList writers[3] = {QMPlay2GUI.getModules("videoWriters", 5), QMPlay2GUI.getModules("audioWriters", 5), QMPlay2GUI.getModules("decoders", 7)};
		QVector<QPair<Module *, Module::Info> > pluginsInstances[3];
		for (int m = 0; m < 3; ++m)
			pluginsInstances[m].fill(QPair<Module *, Module::Info >(), writers[m].size());
		foreach (Module *module, QMPlay2Core.getPluginsInstance())
			foreach (const Module::Info &moduleInfo, module->getModulesInfo())
				for (int m = 0; m < 3; ++m)
				{
					const int mIdx = writers[m].indexOf(moduleInfo.name);
					if (mIdx > -1)
						pluginsInstances[m][mIdx] = qMakePair(module, moduleInfo);
				}
		for (int m = 0; m < 3; ++m)
			for (int i = 0; i < pluginsInstances[m].size(); i++)
			{
				QListWidgetItem *wI = new QListWidgetItem(writers[m][i]);
				wI->setData(Qt::UserRole, pluginsInstances[m][i].first->name());
				wI->setIcon(QMPlay2GUI.getIcon(pluginsInstances[m][i].second.img.isNull() ? pluginsInstances[m][i].first->image() : pluginsInstances[m][i].second.img));
				page2->modulesList[m]->list->addItem(wI);
				if (writers[m][i] == lastM[m])
					page2->modulesList[m]->list->setCurrentItem(wI);
			}
		for (int m = 0; m < 3; ++m)
			if (page2->modulesList[m]->list->currentRow() < 0)
				page2->modulesList[m]->list->setCurrentRow(0);
	}
	else if (idx == 2)
		for (int m = 0; m < 3; ++m)
		{
			QListWidgetItem *currI = page2->modulesList[m]->list->currentItem();
			if (currI)
				lastM[m] = currI->text();
			page2->modulesList[m]->list->clear();
		}
}
void SettingsWidget::openModuleSettings(QListWidgetItem *wI)
{
	QList<QListWidgetItem *> items = page3->listW->findItems(wI->data(Qt::UserRole).toString(), Qt::MatchExactly);
	if (items.size())
	{
		page3->listW->setCurrentItem(items[0]);
		tabW->setCurrentIndex(2);
	}
}
void SettingsWidget::moveModule()
{
	QToolButton *tB = qobject_cast<QToolButton *>(sender());
	if (tB)
	{
		const bool moveDown = tB->arrowType() == Qt::DownArrow;
		QListWidget *mL = NULL;
		if (tB->parent() == page2->modulesList[0]->buttonsW)
			mL = page2->modulesList[0]->list;
		else if (tB->parent() == page2->modulesList[1]->buttonsW)
			mL = page2->modulesList[1]->list;
		else if (tB->parent() == page2->modulesList[2]->buttonsW)
			mL = page2->modulesList[2]->list;
		if (mL)
		{
			int row = mL->currentRow();
			if (row > -1)
			{
				QListWidgetItem *item = mL->takeItem(row);
				mL->clearSelection();
				if (moveDown)
					++row;
				else
					--row;
				mL->insertItem(row, item);
				mL->setCurrentItem(item);
			}
		}
	}
}
void SettingsWidget::chooseScreenshotDir()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"), page1->screenshotE->text());
	if (!dir.isEmpty())
		page1->screenshotE->setText(dir);
}
void SettingsWidget::page2EnableOrDisable()
{
	page2->samplerateB->setEnabled(page2->forceSamplerate->isChecked());
	page2->channelsB->setEnabled(page2->forceChannels->isChecked());
}
void SettingsWidget::setAppearance()
{
	Appearance(this).exec();
}
void SettingsWidget::clearCoversCache()
{
	if (QMessageBox::question(this, tr("Confirm clearing the cache covers"), tr("Do you want to delete all cached covers?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		QDir dir(QMPlay2Core.getSettingsDir());
		if (dir.cd("Covers"))
		{
			foreach (const QString &fileName, dir.entryList(QDir::Files))
				QFile::remove(dir.absolutePath() + "/" + fileName);
			if (dir.cdUp())
				dir.rmdir("Covers");
		}
	}
}
void SettingsWidget::resetSettings()
{
	if (QMessageBox::question(this, tr("Confirm settings deletion"), tr("Do you really want to clear all settings?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		QMPlay2GUI.removeSettings = true;
		restartApp();
	}
}
