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

#include <Extensions.hpp>

#include <Downloader.hpp>
#include <YouTube.hpp>
#ifdef USE_LASTFM
	#include <LastFM.hpp>
#endif
#include <Radio.hpp>
#ifdef USE_MPRIS2
	#include <MPRIS2.hpp>
#endif

#include <QCoreApplication>

Extensions::Extensions() :
	Module("Extensions"),
	downloader(":/downloader"), youtube(":/youtube"), radio(":/radio")
{
	moduleImg = QImage(":/Extensions");

	downloader.setText("Path", ":/downloader");
	youtube.setText("Path", ":/youtube");
	radio.setText("Path", ":/radio");
#ifdef USE_LASTFM
	lastfm = QImage(":/lastfm");
	lastfm.setText("Path", ":/lastfm");
#endif

	init("YouTube/ShowAdditionalInfo", false);
	init("YouTube/MultiStream", true);
	init("YouTube/Subtitles", true);
	init("YouTube/youtubedl", QString());
	init("YouTube/ItagVideoList", YouTubeW::getQualityPresetString(YouTubeW::_1080p60));
	init("YouTube/ItagAudioList", QStringList() << "251" << "171" << "140");
	init("YouTube/ItagList", QStringList() << "22" << "43" << "18");

#ifdef USE_LASTFM
	init("LastFM/DownloadCovers", true);
	init("LastFM/AllowBigCovers", true);
	init("LastFM/UpdateNowPlayingAndScrobble", false);
	init("LastFM/Login", QString());
	init("LastFM/Password", QString());
#endif

#ifdef USE_MPRIS2
	init("MPRIS2/Enabled", true);
#endif
}

QList<Extensions::Info> Extensions::getModulesInfo(const bool) const
{
	QList<Info> modulesInfo;
	modulesInfo += Info(DownloaderName, QMPLAY2EXTENSION, downloader);
	modulesInfo += Info(YouTubeName, QMPLAY2EXTENSION, youtube);
#ifdef USE_LASTFM
	modulesInfo += Info(LastFMName, QMPLAY2EXTENSION, lastfm);
#endif
	modulesInfo += Info(RadioName, QMPLAY2EXTENSION, radio);
#ifdef USE_MPRIS2
	modulesInfo += Info(MPRIS2Name, QMPLAY2EXTENSION);
#endif
	return modulesInfo;
}
void *Extensions::createInstance(const QString &name)
{
	if (name == DownloaderName)
		return new Downloader(*this);
	else if (name == YouTubeName)
		return new YouTube(*this);
#ifdef USE_LASTFM
	else if (name == LastFMName)
		return static_cast<QMPlay2Extensions *>(new LastFM(*this));
#endif
	else if (name == RadioName)
		return static_cast<QMPlay2Extensions *>(new Radio(*this));
#ifdef USE_MPRIS2
	else if (name == MPRIS2Name)
		return static_cast<QMPlay2Extensions *>(new MPRIS2(*this));
#endif
	return NULL;
}

Extensions::SettingsWidget *Extensions::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(Extensions)

/**/

#include "LineEdit.hpp"

#include <QCryptographicHash>
#include <QGridLayout>
#include <QListWidget>
#include <QToolButton>
#include <QFileDialog>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	QGridLayout *layout;

#ifdef USE_MPRIS2
	MPRIS2B = new QCheckBox(tr("Using the program via MPRIS2 interface"));
	MPRIS2B->setChecked(sets().getBool("MPRIS2/Enabled"));
#endif

	const ItagNames itagVideoNames = YouTube::getItagNames(sets().getStringList("YouTube/ItagVideoList"), YouTube::MEDIA_VIDEO);
	const ItagNames itagAudioNames = YouTube::getItagNames(sets().getStringList("YouTube/ItagAudioList"), YouTube::MEDIA_AUDIO);
	const ItagNames itagNames = YouTube::getItagNames(sets().getStringList("YouTube/ItagList"), YouTube::MEDIA_AV);

	QGroupBox *youTubeB = new QGroupBox("YouTube");

	additionalInfoB = new QCheckBox(tr("Show additional search information"));
	additionalInfoB->setChecked(sets().getBool("YouTube/ShowAdditionalInfo"));

	multiStreamB = new QCheckBox(tr("Use different audio and video streams"));
	multiStreamB->setChecked(sets().getBool("YouTube/MultiStream"));
	connect(multiStreamB, SIGNAL(clicked(bool)), this, SLOT(enableItagLists(bool)));

	subtitlesB = new QCheckBox(tr("Display subtitles if available"));
	subtitlesB->setToolTip(tr("Displays subtitles from YouTube. Follows default subtitles language and QMPlay2 language."));
	subtitlesB->setChecked(sets().getBool("YouTube/Subtitles"));

	QLabel *youtubedlL = new QLabel(tr("Path to the 'youtube-dl' application") + ": ");

	youtubedlE = new LineEdit;
	youtubedlE->setPlaceholderText("youtube-dl");
	youtubedlE->setText(sets().getString("YouTube/youtubedl"));

	youtubedlBrowseB = new QToolButton;
	youtubedlBrowseB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
	youtubedlBrowseB->setToolTip(tr("Browse"));
	connect(youtubedlBrowseB, SIGNAL(clicked()), this, SLOT(browseYoutubedl()));


	QLabel *itagL = new QLabel(tr("Priority of default video/audio quality") + ": ");

	itagLW = new QListWidget;
	itagLW->setDragDropMode(QListWidget::InternalMove);
	itagLW->setSelectionMode(QListWidget::ExtendedSelection);

	QLabel *itagVideoL = new QLabel(tr("Priority of default video quality") + ": ");

	itagVideoLW = new QListWidget;
	itagVideoLW->setDragDropMode(QListWidget::InternalMove);
	itagVideoLW->setSelectionMode(QListWidget::ExtendedSelection);

	QLabel *itagAudioL = new QLabel(tr("Priority of default audio quality") + ": ");

	itagAudioLW = new QListWidget;
	itagAudioLW->setDragDropMode(QListWidget::InternalMove);
	itagAudioLW->setSelectionMode(QListWidget::ExtendedSelection);

	for (int i = 0; i < itagVideoNames.first.count(); ++i)
	{
		QListWidgetItem *lWI = new QListWidgetItem(itagVideoLW);
		lWI->setText(itagVideoNames.first[i]);
		lWI->setData(Qt::UserRole, itagVideoNames.second[i]);
	}
	for (int i = 0; i < itagAudioNames.first.count(); ++i)
	{
		QListWidgetItem *lWI = new QListWidgetItem(itagAudioLW);
		lWI->setText(itagAudioNames.first[i]);
		lWI->setData(Qt::UserRole, itagAudioNames.second[i]);
	}
	for (int i = 0; i < itagNames.first.count(); ++i)
	{
		QListWidgetItem *lWI = new QListWidgetItem(itagLW);
		lWI->setText(itagNames.first[i]);
		lWI->setData(Qt::UserRole, itagNames.second[i]);
	}

	enableItagLists(multiStreamB->isChecked());

	QGridLayout *itagLayout = new QGridLayout;
	itagLayout->addWidget(itagVideoL, 0, 0, 1, 1);
	itagLayout->addWidget(itagVideoLW, 1, 0, 1, 1);
	itagLayout->addWidget(itagAudioL, 0, 1, 1, 1);
	itagLayout->addWidget(itagAudioLW, 1, 1, 1, 1);
	itagLayout->addWidget(itagL, 0, 2, 1, 1);
	itagLayout->addWidget(itagLW, 1, 2, 1, 1);
	itagLayout->setMargin(0);


	layout = new QGridLayout(youTubeB);
	layout->addWidget(additionalInfoB, 0, 0, 1, 3);
	layout->addWidget(multiStreamB, 1, 0, 1, 3);
	layout->addWidget(subtitlesB, 2, 0, 1, 3);
	layout->addWidget(youtubedlL, 3, 0, 1, 1);
	layout->addWidget(youtubedlE, 3, 1, 1, 1);
	layout->addWidget(youtubedlBrowseB, 3, 2, 1, 1);
	layout->addLayout(itagLayout, 4, 0, 1, 3);
	layout->setMargin(2);

#ifdef USE_LASTFM
	QGroupBox *lastFMB = new QGroupBox("LastFM");

	downloadCoversGB = new QGroupBox(tr("Downloads covers"));
	downloadCoversGB->setCheckable(true);
	downloadCoversGB->setChecked(sets().getBool("LastFM/DownloadCovers"));

	allowBigCovers = new QCheckBox(tr("Allow to download big covers"));
	allowBigCovers->setChecked(sets().getBool("LastFM/AllowBigCovers"));

	layout = new QGridLayout(downloadCoversGB);
	layout->addWidget(allowBigCovers);
	layout->setMargin(3);

	updateNowPlayingAndScrobbleB = new QCheckBox(tr("Scrobble"));
	updateNowPlayingAndScrobbleB->setChecked(sets().getBool("LastFM/UpdateNowPlayingAndScrobble"));

	loginE = new LineEdit;
	loginE->setPlaceholderText(tr("User name"));
	loginE->setText(sets().getString("LastFM/Login"));

	passwordE = new LineEdit;
	passwordE->setEchoMode(LineEdit::Password);
	passwordE->setPlaceholderText(sets().getString("LastFM/Password").isEmpty() ? tr("Password") : tr("Last password"));
	connect(passwordE, SIGNAL(textEdited(const QString &)), this, SLOT(passwordEdited()));

	loginPasswordEnable(updateNowPlayingAndScrobbleB->isChecked());

	connect(updateNowPlayingAndScrobbleB, SIGNAL(toggled(bool)), this, SLOT(loginPasswordEnable(bool)));

	layout = new QGridLayout(lastFMB);
	layout->addWidget(downloadCoversGB);
	layout->addWidget(updateNowPlayingAndScrobbleB);
	layout->addWidget(loginE);
	layout->addWidget(passwordE);
	layout->setMargin(2);
#endif

	QGridLayout *mainLayout = new QGridLayout(this);
	mainLayout->setProperty("NoVHSpacer", true);
#ifdef USE_MPRIS2
	mainLayout->addWidget(MPRIS2B);
#endif
	mainLayout->addWidget(youTubeB);
#ifdef USE_LASTFM
	mainLayout->addWidget(lastFMB);
#endif
}

void ModuleSettingsWidget::enableItagLists(bool b)
{
	itagVideoLW->setEnabled(b);
	itagAudioLW->setEnabled(b);
	itagLW->setDisabled(b);
}
void ModuleSettingsWidget::browseYoutubedl()
{
	const QString filePath = QFileDialog::getOpenFileName(this, tr("Choose 'youtube-dl' application"), youtubedlE->text());
	if (!filePath.isEmpty())
		youtubedlE->setText(filePath);
}
#ifdef USE_LASTFM
void ModuleSettingsWidget::loginPasswordEnable(bool checked)
{
	loginE->setEnabled(checked);
	passwordE->setEnabled(checked);
}
void ModuleSettingsWidget::passwordEdited()
{
	passwordE->setProperty("edited", true);
}
#endif

void ModuleSettingsWidget::saveSettings()
{
#ifdef USE_MPRIS2
	sets().set("MPRIS2/Enabled", MPRIS2B->isChecked());
#endif

	sets().set("YouTube/ShowAdditionalInfo", additionalInfoB->isChecked());
	sets().set("YouTube/MultiStream", multiStreamB->isChecked());
	sets().set("YouTube/Subtitles", subtitlesB->isChecked());
	sets().set("YouTube/youtubedl", youtubedlE->text());

	QStringList itagsVideo, itagsAudio, itags;
	for (int i = 0; i < itagVideoLW->count(); ++i)
		itagsVideo += itagVideoLW->item(i)->data(Qt::UserRole).toString();
	for (int i = 0; i < itagAudioLW->count(); ++i)
		itagsAudio += itagAudioLW->item(i)->data(Qt::UserRole).toString();
	for (int i = 0; i < itagLW->count(); ++i)
		itags += itagLW->item(i)->data(Qt::UserRole).toString();
	sets().set("YouTube/ItagVideoList", itagsVideo);
	sets().set("YouTube/ItagAudioList", itagsAudio);
	sets().set("YouTube/ItagList", itags);

#ifdef USE_LASTFM
	sets().set("LastFM/DownloadCovers", downloadCoversGB->isChecked());
	sets().set("LastFM/AllowBigCovers", allowBigCovers->isChecked());
	sets().set("LastFM/UpdateNowPlayingAndScrobble", updateNowPlayingAndScrobbleB->isChecked() && !loginE->text().isEmpty());
	sets().set("LastFM/Login", loginE->text());
	if (loginE->text().isEmpty())
		sets().set("LastFM/Password", QString());
	else if (!passwordE->text().isEmpty() && passwordE->property("edited").toBool())
		sets().set("LastFM/Password", QString(QCryptographicHash::hash(passwordE->text().toUtf8(), QCryptographicHash::Md5).toHex()));
#endif
}
