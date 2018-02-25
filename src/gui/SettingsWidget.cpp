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

#include <SettingsWidget.hpp>

#include <DeintSettingsW.hpp>
#include <OtherVFiltersW.hpp>
#include <OSDSettingsW.hpp>
#include <Functions.hpp>
#include <YouTubeDL.hpp>
#include <Notifies.hpp>
#include <Main.hpp>

#include <QStandardPaths>
#include <QStyleFactory>
#include <QRadioButton>
#include <QApplication>
#include <QFormLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QScrollArea>
#include <QToolButton>
#include <QGridLayout>
#include <QMainWindow>
#include <QBoxLayout>
#include <QTextCodec>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QBuffer>
#include <QLabel>

#include <KeyBindingsDialog.hpp>
#include <Appearance.hpp>
#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Module.hpp>

#include "ui_SettingsGeneral.h"
#include "ui_SettingsPlayback.h"
#include "ui_SettingsPlaybackModulesList.h"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_ANDROID)
	#define ICONS_FROM_THEME
#endif

#ifdef Q_OS_WIN
	#include <windows.h>
#endif

class Page3 : public QWidget
{
public:
	inline Page3() :
		module(nullptr)
	{}

	QListWidget *listW;
	QScrollArea *scrollA;
	Module *module;
};
class Page4 : public OSDSettingsW
{
public:
	inline Page4() :
		OSDSettingsW("Subtitles")
	{}

	QGroupBox *toAssGB;
	QCheckBox *colorsAndBordersB, *marginsAndAlignmentB, *fontsB, *overridePlayResB;
};
class Page5 : public OSDSettingsW
{
public:
	inline Page5() :
		OSDSettingsW("OSD")
	{}

	QCheckBox *enabledB;
};
class Page6 : public QScrollArea
{
public:
	inline Page6()
	{
		setWidgetResizable(true);
		setFrameShape(QFrame::NoFrame);
	}

	DeintSettingsW *deintSettingsW;
	QGroupBox *videoEqContainer;
	OtherVFiltersW *otherVFiltersW;
};

template<typename TextWidget>
static inline void appendColon(TextWidget *tw)
{
	tw->setText(tw->text() + ": ");
}

/**/

void SettingsWidget::InitSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();

	QTextCodec *codec = QTextCodec::codecForLocale();
	QMPSettings.init("FallbackSubtitlesEncoding", codec ? codec->name().constData() : "System");

	QMPSettings.init("AudioLanguage", QString());
	QMPSettings.init("SubtitlesLanguage", QString());
	QMPSettings.init("screenshotPth", QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QDir::homePath()));
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
	QMPSettings.init("BlurCovers", true);
	QMPSettings.init("ShowDirCovers", true);
	QMPSettings.init("AutoOpenVideoWindow", true);
	QMPSettings.init("AutoRestoreMainWindowOnVideo", true);
	if (!QMPSettings.contains("AutoUpdates"))
		QMPSettings.init("AutoUpdates", !QFile::exists(QMPlay2Core.getShareDir() + "noautoupdates"));
	QMPSettings.init("MainWidget/TabPositionNorth", false);
#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
	QMPSettings.init("AllowOnlyOneInstance", false);
#endif
	QMPSettings.init("HideArtistMetadata", false);
	QMPSettings.init("DisplayOnlyFileName", false);
	QMPSettings.init("RestoreRepeatMode", false);
	QMPSettings.init("StillImages", false);
	QMPSettings.init("TrayNotifiesDefault", false);
	QMPSettings.init("AutoDelNonGroupEntries", false);
	QMPSettings.init("Proxy/Use", false);
	QMPSettings.init("Proxy/Host", QString());
	QMPSettings.init("Proxy/Port", 80);
	QMPSettings.init("Proxy/Login", false);
	QMPSettings.init("Proxy/User", QString());
	QMPSettings.init("Proxy/Password", QString());
	QMPSettings.init("ShortSeek", 5);
	QMPSettings.init("LongSeek", 30);
	QMPSettings.init("AVBufferLocal", 100);
	QMPSettings.init("AVBufferNetwork", 50000);
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
	QMPSettings.init("LeftMouseTogglePlay", false);
	QMPSettings.init("AccurateSeek", Qt::PartiallyChecked);
	QMPSettings.init("UnpauseWhenSeeking", false);
	QMPSettings.init("StoreARatioAndZoom", false);
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
	for (QAction *act : QMPlay2GUI.menuBar->playback->audioChannels->actions())
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

SettingsWidget::SettingsWidget(int page, const QString &moduleName, QWidget *videoEq) :
	videoEq(videoEq), videoEqOriginalParent(videoEq->parentWidget()),
	wasShow(false),
	moduleIndex(0)
{
	setWindowFlags(Qt::Window);
	setWindowTitle(tr("Settings"));
	setAttribute(Qt::WA_DeleteOnClose);

	Settings &QMPSettings = QMPlay2Core.getSettings();

	tabW = new QTabWidget;

	QPushButton *applyB = new QPushButton;
	applyB->setText(tr("Apply"));
	connect(applyB, SIGNAL(clicked()), this, SLOT(apply()));

	QPushButton *closeB = new QPushButton;
	closeB->setText(tr("Close"));
	closeB->setShortcut(QKeySequence("Escape"));
	connect(closeB, SIGNAL(clicked()), this, SLOT(close()));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(tabW, 0, 0, 1, 3);
	layout->addWidget(applyB, 1, 1, 1, 1);
	layout->addWidget(closeB, 1, 2, 1, 1);
	layout->setMargin(2);

	/* Page 1 */
	{
		QWidget *page1Widget = new QWidget;
		page1 = new Ui::GeneralSettings;
		page1->setupUi(page1Widget);

		appendColon(page1->langL);
		appendColon(page1->styleL);
		appendColon(page1->encodingL);
		appendColon(page1->audioLangL);
		appendColon(page1->subsLangL);
		appendColon(page1->screenshotL);
		appendColon(page1->profileL);

		tabW->addTab(page1Widget, tr("General settings"));

		int idx;

		page1->langBox->addItem("English", "en");
		page1->langBox->setCurrentIndex(0);
		const QStringList langs = QMPlay2Core.getLanguages();
		for (int i = 0; i < langs.count(); i++)
		{
			page1->langBox->addItem(QMPlay2Core.getLongFromShortLanguage(langs[i]), langs[i]);
			if (QMPlay2Core.getLanguage() == langs[i])
				page1->langBox->setCurrentIndex(i + 1);
		}

		page1->styleBox->addItems(QStyleFactory::keys());
		idx = page1->styleBox->findText(QApplication::style()->objectName(), Qt::MatchFixedString);
		if (idx > -1 && idx < page1->styleBox->count())
			page1->styleBox->setCurrentIndex(idx);
		connect(page1->styleBox, SIGNAL(currentIndexChanged(int)), this, SLOT(chStyle()));

		QStringList encodings;
		for (const QByteArray &item : QTextCodec::availableCodecs())
			encodings += QTextCodec::codecForName(item)->name();
		encodings.removeDuplicates();
		page1->encodingB->addItems(encodings);
		idx = page1->encodingB->findText(QMPSettings.getByteArray("FallbackSubtitlesEncoding"));
		if (idx > -1)
			page1->encodingB->setCurrentIndex(idx);

		const QString audioLang = QMPSettings.getString("AudioLanguage");
		const QString subsLang = QMPSettings.getString("SubtitlesLanguage");
		page1->audioLangB->addItem(tr("Default or first stream"));
		page1->subsLangB->addItem(tr("Default or first stream"));
		for (const QString &lang : QMPlay2Core.getLanguagesMap())
		{
			page1->audioLangB->addItem(lang);
			page1->subsLangB->addItem(lang);
			if (lang == audioLang)
				page1->audioLangB->setCurrentIndex(page1->audioLangB->count() - 1);
			if (lang == subsLang)
				page1->subsLangB->setCurrentIndex(page1->subsLangB->count() - 1);
		}
		{
			const QString currentProfile = QSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat).value("Profile").toString();
			page1->profileB->addItem(tr("Default"));
			for (const QString &profile : QDir(QMPlay2Core.getSettingsDir() + "Profiles/").entryList(QDir::Dirs | QDir::NoDotAndDotDot))
			{
				page1->profileB->addItem(profile);
				if (profile == currentProfile)
					page1->profileB->setCurrentIndex(page1->profileB->count() - 1);
			}
			connect(page1->profileB, SIGNAL(currentIndexChanged(int)), this, SLOT(profileListIndexChanged(int)));

			page1->profileRemoveB->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
			page1->profileRemoveB->setEnabled(page1->profileB->currentIndex() != 0);
			connect(page1->profileRemoveB, SIGNAL(clicked()), this, SLOT(removeProfile()));
		}

		page1->screenshotE->setText(QMPSettings.getString("screenshotPth"));
		page1->screenshotFormatB->setCurrentIndex(page1->screenshotFormatB->findText(QMPSettings.getString("screenshotFormat")));
		page1->screenshotB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
		connect(page1->screenshotB, SIGNAL(clicked()), this, SLOT(chooseScreenshotDir()));

		connect(page1->setAppearanceB, SIGNAL(clicked()), this, SLOT(setAppearance()));
		connect(page1->setKeyBindingsB, SIGNAL(clicked()), this, SLOT(setKeyBindings()));

#ifdef ICONS_FROM_THEME
		page1->iconsFromTheme->setChecked(QMPSettings.getBool("IconsFromTheme"));
#else
		delete page1->iconsFromTheme;
		page1->iconsFromTheme = nullptr;
#endif

		page1->showCoversGB->setChecked(QMPSettings.getBool("ShowCovers"));
		page1->blurCoversB->setChecked(QMPSettings.getBool("BlurCovers"));
		page1->showDirCoversB->setChecked(QMPSettings.getBool("ShowDirCovers"));

		page1->autoOpenVideoWindowB->setChecked(QMPSettings.getBool("AutoOpenVideoWindow"));
		page1->autoRestoreMainWindowOnVideoB->setChecked(QMPSettings.getBool("AutoRestoreMainWindowOnVideo"));

		page1->autoUpdatesB->setChecked(QMPSettings.getBool("AutoUpdates"));
#ifndef UPDATER
		page1->autoUpdatesB->setText(tr("Automatically check for updates"));
#endif

		if (Notifies::hasBoth())
			page1->trayNotifiesDefault->setChecked(QMPSettings.getBool("TrayNotifiesDefault"));
		else
		{
			delete page1->trayNotifiesDefault;
			page1->trayNotifiesDefault = nullptr;
		}

		page1->autoDelNonGroupEntries->setChecked(QMPSettings.getBool("AutoDelNonGroupEntries"));

		page1->tabsNorths->setChecked(QMPSettings.getBool("MainWidget/TabPositionNorth"));

#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
		page1->allowOnlyOneInstance->setChecked(QMPSettings.getBool("AllowOnlyOneInstance"));
#else
		delete page1->allowOnlyOneInstance;
		page1->allowOnlyOneInstance = nullptr;
#endif

		page1->hideArtistMetadata->setChecked(QMPSettings.getBool("HideArtistMetadata"));
		page1->displayOnlyFileName->setChecked(QMPSettings.getBool("DisplayOnlyFileName"));
		page1->restoreRepeatMode->setChecked(QMPSettings.getBool("RestoreRepeatMode"));
		page1->stillImages->setChecked(QMPSettings.getBool("StillImages"));

		page1->proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));
		page1->proxyHostE->setText(QMPSettings.getString("Proxy/Host"));
		page1->proxyPortB->setValue(QMPSettings.getInt("Proxy/Port"));
		page1->proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));
		page1->proxyUserE->setText(QMPSettings.getString("Proxy/User"));
		page1->proxyPasswordE->setText(QByteArray::fromBase64(QMPSettings.getByteArray("Proxy/Password")));

		const QIcon viewRefresh = QMPlay2Core.getIconFromTheme("view-refresh");
		page1->clearCoversCache->setIcon(viewRefresh);
		connect(page1->clearCoversCache, SIGNAL(clicked()), this, SLOT(clearCoversCache()));
		page1->removeYtDlB->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
		connect(page1->removeYtDlB, SIGNAL(clicked()), this, SLOT(removeYouTubeDl()));
		page1->resetSettingsB->setIcon(viewRefresh);
		connect(page1->resetSettingsB, SIGNAL(clicked()), this, SLOT(resetSettings()));
	}

	/* Page 2 */
	{
		QWidget *page2Widget = new QWidget;
		page2 = new Ui::PlaybackSettings;
		page2->setupUi(page2Widget);

		appendColon(page2->shortSeekL);
		appendColon(page2->longSeekL);
		appendColon(page2->bufferLocalL);
		appendColon(page2->bufferNetworkL);
		appendColon(page2->backwardBufferNetworkL);
		appendColon(page2->playIfBufferedL);
		appendColon(page2->maxVolL);
		appendColon(page2->forceSamplerate);
		appendColon(page2->forceChannels);

		page2->shortSeekB->setSuffix(" " + page2->shortSeekB->suffix());
		page2->longSeekB->setSuffix(" " + page2->longSeekB->suffix());
		page2->playIfBufferedB->setSuffix(" " + page2->playIfBufferedB->suffix());
		page2->replayGainPreamp->setPrefix(page2->replayGainPreamp->prefix() + ": ");

		tabW->addTab(page2Widget, tr("Playback settings"));

		page2->shortSeekB->setValue(QMPSettings.getInt("ShortSeek"));
		page2->longSeekB->setValue(QMPSettings.getInt("LongSeek"));
		page2->bufferLocalB->setValue(QMPSettings.getInt("AVBufferLocal"));
		page2->bufferNetworkB->setValue(QMPSettings.getInt("AVBufferNetwork"));
		page2->backwardBufferNetworkB->setCurrentIndex(QMPSettings.getUInt("BackwardBuffer"));
		page2->playIfBufferedB->setValue(QMPSettings.getDouble("PlayIfBuffered"));
		page2->maxVolB->setValue(QMPSettings.getInt("MaxVol"));

		page2->forceSamplerate->setChecked(QMPSettings.getBool("ForceSamplerate"));
		page2->samplerateB->setValue(QMPSettings.getInt("Samplerate"));
		connect(page2->forceSamplerate, SIGNAL(toggled(bool)), page2->samplerateB, SLOT(setEnabled(bool)));
		page2->samplerateB->setEnabled(page2->forceSamplerate->isChecked());

		page2->forceChannels->setChecked(QMPSettings.getBool("ForceChannels"));
		page2->channelsB->setValue(QMPSettings.getInt("Channels"));
		connect(page2->forceChannels, SIGNAL(toggled(bool)), page2->channelsB, SLOT(setEnabled(bool)));
		page2->channelsB->setEnabled(page2->forceChannels->isChecked());

		page2->replayGain->setChecked(QMPSettings.getBool("ReplayGain/Enabled"));
		page2->replayGainAlbum->setChecked(QMPSettings.getBool("ReplayGain/Album"));
		page2->replayGainPreventClipping->setChecked(QMPSettings.getBool("ReplayGain/PreventClipping"));
		page2->replayGainPreamp->setValue(QMPSettings.getDouble("ReplayGain/Preamp"));

		page2->wheelActionB->setChecked(QMPSettings.getBool("WheelAction"));
		page2->wheelSeekB->setChecked(QMPSettings.getBool("WheelSeek"));
		page2->wheelVolumeB->setChecked(QMPSettings.getBool("WheelVolume"));

		page2->storeARatioAndZoomB->setChecked(QMPSettings.getBool("StoreARatioAndZoom"));
		connect(page2->storeARatioAndZoomB, &QCheckBox::toggled, this, [this](bool checked) {
			if (checked)
			{
				page2->keepZoom->setChecked(true);
				page2->keepARatio->setChecked(true);
			}
		});

		page2->keepZoom->setChecked(QMPSettings.getBool("KeepZoom"));
		connect(page2->keepZoom, &QCheckBox::toggled, this, [this](bool checked) {
			if (!checked && !page2->keepARatio->isChecked())
			{
				page2->storeARatioAndZoomB->setChecked(false);
			}
		});

		page2->keepARatio->setChecked(QMPSettings.getBool("KeepARatio"));
		connect(page2->keepARatio, &QCheckBox::toggled, this, [this](bool checked) {
			if (!checked && !page2->keepZoom->isChecked())
			{
				page2->storeARatioAndZoomB->setChecked(false);
			}
		});

		page2->showBufferedTimeOnSlider->setChecked(QMPSettings.getBool("ShowBufferedTimeOnSlider"));
		page2->savePos->setChecked(QMPSettings.getBool("SavePos"));
		page2->keepSubtitlesDelay->setChecked(QMPSettings.getBool("KeepSubtitlesDelay"));
		page2->keepSubtitlesScale->setChecked(QMPSettings.getBool("KeepSubtitlesScale"));
		page2->keepVideoDelay->setChecked(QMPSettings.getBool("KeepVideoDelay"));
		page2->keepSpeed->setChecked(QMPSettings.getBool("KeepSpeed"));
		page2->syncVtoA->setChecked(QMPSettings.getBool("SyncVtoA"));
		page2->silence->setChecked(QMPSettings.getBool("Silence"));
		page2->restoreVideoEq->setChecked(QMPSettings.getBool("RestoreVideoEqualizer"));
		page2->ignorePlaybackError->setChecked(QMPSettings.getBool("IgnorePlaybackError"));
		page2->leftMouseTogglePlay->setChecked(QMPSettings.getBool("LeftMouseTogglePlay"));

		page2->accurateSeekB->setCheckState((Qt::CheckState)QMPSettings.getInt("AccurateSeek"));
		page2->accurateSeekB->setToolTip(tr("Slower, but more accurate seeking.\nPartially checked doesn't affect seeking on slider."));

		page2->unpauseWhenSeekingB->setChecked(QMPSettings.getBool("UnpauseWhenSeeking"));

		const QString modulesListTitle[3] = {
			tr("Video output priority"),
			tr("Audio output priority"),
			tr("Decoders priority")
		};
		for (int m = 0; m < 3; ++m)
		{
			QGroupBox *groupB = new QGroupBox(modulesListTitle[m]);
			Ui::ModulesList *ml = new Ui::ModulesList;
			ml->setupUi(groupB);
			connect(ml->list, SIGNAL(itemDoubleClicked (QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
			connect(ml->moveUp, SIGNAL(clicked()), this, SLOT(moveModule()));
			connect(ml->moveDown, SIGNAL(clicked()), this, SLOT(moveModule()));
			ml->moveUp->setProperty("idx", m);
			ml->moveDown->setProperty("idx", m);
			page2->modulesListLayout->addWidget(groupB);
			page2ModulesList[m] = ml;
		}
	}

	/* Page 3 */
	{
		page3 = new Page3;
		tabW->addTab(page3, tr("Modules"));

		page3->listW = new QListWidget;
		page3->listW->setIconSize({32, 32});
		page3->listW->setMinimumSize(200, 0);
		page3->listW->setMaximumSize(200, 16777215);
		for (Module *module : QMPlay2Core.getPluginsInstance())
		{
			QListWidgetItem *tWI = new QListWidgetItem(module->name());
			tWI->setData(Qt::UserRole, qVariantFromValue((void *)module));
			QString toolTip = "<html>" + tr("Contains") + ":";
			for (const Module::Info &mod : module->getModulesInfo(true))
			{
				const QPixmap moduleIcon = Functions::getPixmapFromIcon(mod.icon, QSize(22, 22), this);
				toolTip += "<p>&nbsp;&nbsp;&nbsp;&nbsp;";
				bool hasIcon = false;
				if (!moduleIcon.isNull())
				{
					QBuffer buffer;
					if (buffer.open(QBuffer::WriteOnly) && moduleIcon.save(&buffer, "PNG"))
					{
						toolTip += "<img width='22' height='22' src='data:image/png;base64, " + buffer.data().toBase64() + "'/> ";
						hasIcon = true;
					}
				}
				if (!hasIcon)
					toolTip += "- ";
				toolTip += mod.name + "</p>";
			}
			toolTip += "</html>";
			tWI->setToolTip(toolTip);
			tWI->setIcon(QMPlay2GUI.getIcon(module->icon()));
			page3->listW->addItem(tWI);
			if (page == 2 && !moduleName.isEmpty() && module->name() == moduleName)
				moduleIndex = page3->listW->count() - 1;
		}

		page3->scrollA = new QScrollArea;
		page3->scrollA->setWidgetResizable(true);
		page3->scrollA->setFrameShape(QFrame::NoFrame);

		QHBoxLayout *layout = new QHBoxLayout(page3);
		layout->setMargin(0);
		layout->setSpacing(1);
		layout->addWidget(page3->listW);
		layout->addWidget(page3->scrollA);
		connect(page3->listW, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(chModule(QListWidgetItem *)));
	}

	/* Page 4 */
	{
		page4 = new Page4;
		tabW->addTab(page4, tr("Subtitles"));

		page4->colorsAndBordersB = new QCheckBox(tr("Colors and borders"));
		page4->colorsAndBordersB->setChecked(QMPSettings.getBool("ApplyToASS/ColorsAndBorders"));

		page4->marginsAndAlignmentB = new QCheckBox(tr("Margins and alignment"));
		page4->marginsAndAlignmentB->setChecked(QMPSettings.getBool("ApplyToASS/MarginsAndAlignment"));

		page4->fontsB = new QCheckBox(tr("Fonts and spacing"));
		page4->fontsB->setChecked(QMPSettings.getBool("ApplyToASS/FontsAndSpacing"));

		page4->overridePlayResB = new QCheckBox(tr("Use the same size"));
		page4->overridePlayResB->setChecked(QMPSettings.getBool("ApplyToASS/OverridePlayRes"));

		page4->toAssGB = new QGroupBox(tr("Apply for ASS/SSA subtitles"));
		page4->toAssGB->setCheckable(true);
		page4->toAssGB->setChecked(QMPSettings.getBool("ApplyToASS/ApplyToASS"));

		QGridLayout *page4ToAssLayout = new QGridLayout(page4->toAssGB);
		page4ToAssLayout->addWidget(page4->colorsAndBordersB, 0, 0, 1, 1);
		page4ToAssLayout->addWidget(page4->marginsAndAlignmentB, 1, 0, 1, 1);
		page4ToAssLayout->addWidget(page4->fontsB, 0, 1, 1, 1);
		page4ToAssLayout->addWidget(page4->overridePlayResB, 1, 1, 1, 1);

		page4->addWidget(page4->toAssGB);
	}

	/* Page 5 */
	{
		page5 = new Page5;
		tabW->addTab(page5, tr("OSD"));

		page5->enabledB = new QCheckBox(tr("OSD enabled"));
		page5->enabledB->setChecked(QMPSettings.getBool("OSD/Enabled"));
		page5->addWidget(page5->enabledB);
	}

	/* Page 6 */
	{
		page6 = new Page6;
		tabW->addTab(page6, tr("Video filters"));

		QWidget *widget = new QWidget;

		QGridLayout *layout = new QGridLayout(widget);
		layout->setMargin(0);

		page6->deintSettingsW = new DeintSettingsW;
		layout->addWidget(page6->deintSettingsW, 0, 0, 1, 2);

		page6->videoEqContainer = new QGroupBox(videoEq->objectName());
		layout->addWidget(page6->videoEqContainer, 1, 0, 2, 1);

		page6->otherVFiltersW = new OtherVFiltersW(false);
		if (!page6->otherVFiltersW->count())
		{
			delete page6->otherVFiltersW;
			page6->otherVFiltersW = nullptr;
		}
		else
		{
			QGroupBox *otherVFiltersContainer = new QGroupBox(tr("Software video filters"));
			QGridLayout *otherVFiltersLayout = new QGridLayout(otherVFiltersContainer);
			otherVFiltersLayout->addWidget(page6->otherVFiltersW);
			connect(page6->otherVFiltersW, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
			layout->addWidget(otherVFiltersContainer, 1, 1, 1, 1);
		}

		OtherVFiltersW *otherHWVFiltersW = new OtherVFiltersW(true);
		if (!otherHWVFiltersW->count())
			delete otherHWVFiltersW;
		else
		{
			QGroupBox *otherHWVFiltersContainer = new QGroupBox(tr("Hardware accelerated video outputs"));
			QGridLayout *otherHWVFiltersLayout = new QGridLayout(otherHWVFiltersContainer);
			otherHWVFiltersLayout->addWidget(otherHWVFiltersW);
			connect(otherHWVFiltersW, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openModuleSettings(QListWidgetItem *)));
			layout->addWidget(otherHWVFiltersContainer, 2, 1, 1, 1);
		}

		page6->setWidget(widget);
	}

	connect(tabW, SIGNAL(currentChanged(int)), this, SLOT(tabCh(int)));
	tabW->setCurrentIndex(page);

	show();
}
SettingsWidget::~SettingsWidget()
{
	for (int i = 0; i < 3; ++i)
		delete page2ModulesList[i];
	delete page2;
	delete page1;
}

void SettingsWidget::setAudioChannels()
{
	page2->forceChannels->setChecked(QMPlay2Core.getSettings().getBool("ForceChannels"));
	page2->channelsB->setValue(QMPlay2Core.getSettings().getInt("Channels"));
}

void SettingsWidget::applyProxy()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	if (!QMPSettings.getBool("Proxy/Use"))
	{
#ifdef Q_OS_WIN
		SetEnvironmentVariableA("http_proxy", nullptr);
#else
		unsetenv("http_proxy");
#endif
	}
	else
	{
		const QString proxyHostName = QMPSettings.getString("Proxy/Host");
		const quint16 proxyPort = QMPSettings.getInt("Proxy/Port");
		QString proxyUser, proxyPassword;
		if (QMPSettings.getBool("Proxy/Login"))
		{
			proxyUser = QMPSettings.getString("Proxy/User");
			proxyPassword = QByteArray::fromBase64(QMPSettings.getByteArray("Proxy/Password"));
		}

		/* Proxy env for FFmpeg */
		QString proxyEnv = QString("http://" + proxyHostName + ':' + QString::number(proxyPort));
		if (!proxyUser.isEmpty())
		{
			QString auth = proxyUser;
			if (!proxyPassword.isEmpty())
				auth += ':' + proxyPassword;
			auth += '@';
			proxyEnv.insert(7, auth);
		}
#ifdef Q_OS_WIN
		SetEnvironmentVariableA("http_proxy", proxyEnv.toLocal8Bit());
#else
		setenv("http_proxy", proxyEnv.toLocal8Bit(), true);
#endif
	}
}

void SettingsWidget::restoreVideoEq()
{
	if (videoEq->parentWidget() != videoEqOriginalParent)
	{
		videoEq->setParent(videoEqOriginalParent);
		videoEq->setGeometry(videoEqOriginalParent->rect());
		videoEqOriginalParent->setEnabled(true);
	}
	if (QLayout *videoEqLayout = page6->videoEqContainer->layout())
		videoEqLayout->deleteLater();
}

void SettingsWidget::restartApp()
{
	QMPlay2GUI.restartApp = true;
	close();
	QMPlay2GUI.mainW->close();
}

inline QString SettingsWidget::getSelectedProfile()
{
	return !page1->profileB->currentIndex() ? "/" : page1->profileB->currentText();
}

void SettingsWidget::showEvent(QShowEvent *)
{
	if (!wasShow)
	{
		QMPlay2GUI.restoreGeometry("SettingsWidget/Geometry", this, 65);
		page3->listW->setCurrentRow(moduleIndex);
		wasShow = true;
	}
}
void SettingsWidget::closeEvent(QCloseEvent *)
{
	QMPlay2Core.getSettings().set("SettingsWidget/Geometry", geometry());
	restoreVideoEq();
}

void SettingsWidget::chStyle()
{
	const QString newStyle = page1->styleBox->currentText().toLower();
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
				QMPlay2Core.setLanguage();
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
			QMPSettings.set("FallbackSubtitlesEncoding", page1->encodingB->currentText().toUtf8());
			QMPSettings.set("AudioLanguage", page1->audioLangB->currentIndex() > 0 ? page1->audioLangB->currentText() : QString());
			QMPSettings.set("SubtitlesLanguage", page1->subsLangB->currentIndex() > 0 ? page1->subsLangB->currentText() : QString());
			QMPSettings.set("screenshotPth", page1->screenshotE->text());
			QMPSettings.set("screenshotFormat", page1->screenshotFormatB->currentText());
			QMPSettings.set("ShowCovers", page1->showCoversGB->isChecked());
			QMPSettings.set("BlurCovers", page1->blurCoversB->isChecked());
			QMPSettings.set("ShowDirCovers", page1->showDirCoversB->isChecked());
			QMPSettings.set("AutoOpenVideoWindow", page1->autoOpenVideoWindowB->isChecked());
			QMPSettings.set("AutoRestoreMainWindowOnVideo", page1->autoRestoreMainWindowOnVideoB->isChecked());
			QMPSettings.set("AutoUpdates", page1->autoUpdatesB->isChecked());
			QMPSettings.set("MainWidget/TabPositionNorth", page1->tabsNorths->isChecked());
#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
			QMPSettings.set("AllowOnlyOneInstance", page1->allowOnlyOneInstance->isChecked());
#endif
			QMPSettings.set("HideArtistMetadata", page1->hideArtistMetadata->isChecked());
			QMPSettings.set("DisplayOnlyFileName", page1->displayOnlyFileName->isChecked());
			QMPSettings.set("RestoreRepeatMode", page1->restoreRepeatMode->isChecked());
			QMPSettings.set("StillImages", page1->stillImages->isChecked());
			if (page1->trayNotifiesDefault)
				QMPSettings.set("TrayNotifiesDefault", page1->trayNotifiesDefault->isChecked());
			QMPSettings.set("AutoDelNonGroupEntries", page1->autoDelNonGroupEntries->isChecked());
			QMPSettings.set("Proxy/Use", page1->proxyB->isChecked() && !page1->proxyHostE->text().isEmpty());
			QMPSettings.set("Proxy/Host", page1->proxyHostE->text());
			QMPSettings.set("Proxy/Port", page1->proxyPortB->value());
			QMPSettings.set("Proxy/Login", page1->proxyLoginB->isChecked() && !page1->proxyUserE->text().isEmpty());
			QMPSettings.set("Proxy/User", page1->proxyUserE->text());
			QMPSettings.set("Proxy/Password", page1->proxyPasswordE->text().toUtf8().toBase64());
			page1->proxyB->setChecked(QMPSettings.getBool("Proxy/Use"));
			page1->proxyLoginB->setChecked(QMPSettings.getBool("Proxy/Login"));
			qobject_cast<QMainWindow *>(QMPlay2GUI.mainW)->setTabPosition(Qt::AllDockWidgetAreas, page1->tabsNorths->isChecked() ? QTabWidget::North : QTabWidget::South);
			applyProxy();

			if (page1->trayNotifiesDefault)
				Notifies::setNativeFirst(!page1->trayNotifiesDefault->isChecked());

			QSettings profileSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat);
			const QString selectedProfile = getSelectedProfile();
			if (selectedProfile != profileSettings.value("Profile", "/").toString())
			{
				profileSettings.setValue("Profile", selectedProfile);
				restartApp();
			}
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
			QMPSettings.set("LeftMouseTogglePlay", page2->leftMouseTogglePlay->isChecked());
			QMPSettings.set("AccurateSeek", page2->accurateSeekB->checkState());
			QMPSettings.set("UnpauseWhenSeeking", page2->unpauseWhenSeekingB->isChecked());
			QMPSettings.set("StoreARatioAndZoom", page2->storeARatioAndZoomB->isChecked());

			QStringList videoWriters, audioWriters, decoders;
			for (QListWidgetItem *wI : page2ModulesList[0]->list->findItems(QString(), Qt::MatchContains))
				videoWriters += wI->text();
			for (QListWidgetItem *wI : page2ModulesList[1]->list->findItems(QString(), Qt::MatchContains))
				audioWriters += wI->text();
			for (QListWidgetItem *wI : page2ModulesList[2]->list->findItems(QString(), Qt::MatchContains))
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
			QMPSettings.set("ApplyToASS/ApplyToASS", page4->toAssGB->isChecked());
			break;
		case 5:
			QMPSettings.set("OSD/Enabled", page5->enabledB->isChecked());
			page5->writeSettings();
			break;
		case 6:
			page6->deintSettingsW->writeSettings();
			if (page6->otherVFiltersW)
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
			QLayout *layout = w->layout();
			layout->setMargin(2);
			if (QFormLayout *fLayout = qobject_cast<QFormLayout *>(layout))
				fLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
			else if (QGridLayout *gLayout = qobject_cast<QGridLayout *>(layout))
			{
				if (!gLayout->property("NoVHSpacer").toBool())
				{
					gLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), gLayout->rowCount(), 0); //vSpacer
					gLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, gLayout->columnCount()); //hSpacer
				}
			}
			page3->scrollA->setWidget(w);
			w->setAutoFillBackground(false);
		}
		else if (page3->scrollA->widget())
			page3->scrollA->widget()->close(); //ustawi się na nullptr po usunięciu (WA_DeleteOnClose)
	}
}
void SettingsWidget::tabCh(int idx)
{
	if (idx == 1 && !page2ModulesList[0]->list->count() && !page2ModulesList[1]->list->count() && !page2ModulesList[2]->list->count())
	{
		const QStringList writers[3] = {QMPlay2Core.getModules("videoWriters", 5), QMPlay2Core.getModules("audioWriters", 5), QMPlay2Core.getModules("decoders", 7)};
		QVector<QPair<Module *, Module::Info>> pluginsInstances[3];
		for (int m = 0; m < 3; ++m)
			pluginsInstances[m].fill(QPair<Module *, Module::Info >(), writers[m].size());
		for (Module *module : QMPlay2Core.getPluginsInstance())
			for (const Module::Info &moduleInfo : module->getModulesInfo())
				for (int m = 0; m < 3; ++m)
				{
					const int mIdx = writers[m].indexOf(moduleInfo.name);
					if (mIdx > -1)
						pluginsInstances[m][mIdx] = {module, moduleInfo};
				}
		for (int m = 0; m < 3; ++m)
		{
			for (int i = 0; i < pluginsInstances[m].size(); i++)
			{
				QListWidgetItem *wI = new QListWidgetItem(writers[m][i]);
				wI->setData(Qt::UserRole, pluginsInstances[m][i].first->name());
				wI->setIcon(QMPlay2GUI.getIcon(pluginsInstances[m][i].second.icon.isNull() ? pluginsInstances[m][i].first->icon() : pluginsInstances[m][i].second.icon));
				page2ModulesList[m]->list->addItem(wI);
				if (writers[m][i] == lastM[m])
					page2ModulesList[m]->list->setCurrentItem(wI);
			}
			if (page2ModulesList[m]->list->currentRow() < 0)
				page2ModulesList[m]->list->setCurrentRow(0);
		}
	}
	else if (idx == 2)
	{
		for (int m = 0; m < 3; ++m)
		{
			QListWidgetItem *currI = page2ModulesList[m]->list->currentItem();
			if (currI)
				lastM[m] = currI->text();
			page2ModulesList[m]->list->clear();
		}
	}

	if (idx != 5)
		restoreVideoEq();
	else
	{
		QGridLayout *videoEqLayout = new QGridLayout(page6->videoEqContainer);
		videoEqLayout->setMargin(0);
		videoEqLayout->addWidget(videoEq);
		videoEqOriginalParent->setDisabled(true);
		videoEq->show();
	}
}
void SettingsWidget::openModuleSettings(QListWidgetItem *wI)
{
	const QList<QListWidgetItem *> items = page3->listW->findItems(wI->data(Qt::UserRole).toString(), Qt::MatchExactly);
	if (!items.isEmpty())
	{
		page3->listW->setCurrentItem(items[0]);
		tabW->setCurrentIndex(2);
	}
}
void SettingsWidget::moveModule()
{
	if (QToolButton *tB = qobject_cast<QToolButton *>(sender()))
	{
		const bool moveDown = tB->arrowType() == Qt::DownArrow;
		const int idx = tB->property("idx").toInt();
		QListWidget *mL = page2ModulesList[idx]->list;
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
void SettingsWidget::chooseScreenshotDir()
{
	const QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory"), page1->screenshotE->text());
	if (!dir.isEmpty())
		page1->screenshotE->setText(dir);
}
void SettingsWidget::setAppearance()
{
	Appearance(this).exec();
}
void SettingsWidget::setKeyBindings()
{
	KeyBindingsDialog(this).exec();
}
void SettingsWidget::clearCoversCache()
{
	if (QMessageBox::question(this, tr("Confirm clearing the cache covers"), tr("Do you want to delete all cached covers?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		QDir dir(QMPlay2Core.getSettingsDir());
		if (dir.cd("Covers"))
		{
			for (const QString &fileName : dir.entryList(QDir::Files))
				QFile::remove(dir.absolutePath() + "/" + fileName);
			if (dir.cdUp())
				dir.rmdir("Covers");
		}
	}
}
void SettingsWidget::removeYouTubeDl()
{
	const QString filePath = YouTubeDL::getFilePath();
	while (QFile::exists(filePath))
	{
		if (QMessageBox::question(this, tr("Confirm \"youtube-dl\" deletion"), tr("Do you want to remove \"youtube-dl\" software?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
			break;
		QFile::remove(filePath);
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
void SettingsWidget::profileListIndexChanged(int index)
{
	page1->profileRemoveB->setEnabled(index != 0);
}
void SettingsWidget::removeProfile()
{
	const QString selectedProfileName = getSelectedProfile();
	const QString selectedProfile = "Profiles/" + selectedProfileName + "/";
	if (selectedProfile == QMPlay2Core.getSettingsProfile())
	{
		QSettings(QMPlay2Core.getSettingsDir() + "Profile.ini", QSettings::IniFormat).setValue("Profile", "/");
		QMPlay2GUI.removeSettings = true;
		restartApp();
	}
	else
	{
		const QString settingsDir = QMPlay2Core.getSettingsDir() + selectedProfile;
		for (const QString &fName : QDir(settingsDir).entryList({"*.ini"}))
			QFile::remove(settingsDir + fName);
		QDir(QMPlay2Core.getSettingsDir()).rmdir(selectedProfile);

		page1->profileB->removeItem(page1->profileB->currentIndex());
		for (QAction *act : QMPlay2GUI.menuBar->options->profilesGroup->actions())
			if (act->text() == selectedProfileName)
			{
				delete act;
				break;
			}
	}
}
