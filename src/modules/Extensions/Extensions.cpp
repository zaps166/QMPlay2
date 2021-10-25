/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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
#ifdef USE_YOUTUBE
    #include <YouTube.hpp>
#endif
#ifdef USE_LASTFM
    #include <LastFM.hpp>
#endif
#ifdef USE_RADIO
    #include <Radio.hpp>
#endif
#ifdef USE_MPRIS2
    #include <MPRIS2.hpp>
#endif
#ifdef USE_LYRICS
    #include <Lyrics.hpp>
#endif
#ifdef USE_MEDIABROWSER
    #include <MediaBrowser.hpp>
#endif

#include <QCoreApplication>
#include <QComboBox>

Extensions::Extensions() :
    Module("Extensions"),
    downloader(":/downloader.svgz")
{
    m_icon = QIcon(":/Extensions.svgz");

#ifdef USE_LASTFM
    lastfm = QIcon(":/lastfm.svgz");
#endif
#ifdef USE_YOUTUBE
    youtube = QIcon(":/youtube.svgz");
#endif
#ifdef USE_RADIO
    radio = QIcon(":/radio.svgz");
#endif

#ifdef USE_YOUTUBE
    init("YouTube/ShowUserName", false);
    init("YouTube/Subtitles", true);
    init("YouTube/SortBy", 0);
#endif

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
#ifdef USE_YOUTUBE
    modulesInfo += Info(YouTubeName, QMPLAY2EXTENSION, youtube);
#endif
#ifdef USE_LASTFM
    modulesInfo += Info(LastFMName, QMPLAY2EXTENSION, lastfm);
#endif
#ifdef USE_RADIO
    modulesInfo += Info(RadioName, QMPLAY2EXTENSION, radio);
#endif
#ifdef USE_LYRICS
    modulesInfo += Info(LyricsName, QMPLAY2EXTENSION);
#endif
#ifdef USE_MEDIABROWSER
    modulesInfo += Info(MediaBrowserName, QMPLAY2EXTENSION);
#endif
#ifdef USE_MPRIS2
    modulesInfo += Info(MPRIS2Name, QMPLAY2EXTENSION);
#endif
    return modulesInfo;
}
void *Extensions::createInstance(const QString &name)
{
    if (name == DownloaderName)
        return static_cast<QMPlay2Extensions *>(new Downloader(*this));
#ifdef USE_YOUTUBE
    else if (name == YouTubeName)
        return static_cast<QMPlay2Extensions *>(new YouTube(*this));
#endif
#ifdef USE_LASTFM
    else if (name == LastFMName)
        return static_cast<QMPlay2Extensions *>(new LastFM(*this));
#endif
#ifdef USE_RADIO
    else if (name == RadioName)
        return static_cast<QMPlay2Extensions *>(new Radio(*this));
#endif
#ifdef USE_LYRICS
    else if (name == LyricsName)
        return static_cast<QMPlay2Extensions *>(new Lyrics(*this));
#endif
#ifdef USE_MEDIABROWSER
    else if (name == MediaBrowserName)
        return static_cast<QMPlay2Extensions *>(new MediaBrowser(*this));
#endif
#ifdef USE_MPRIS2
    else if (name == MPRIS2Name)
        return static_cast<QMPlay2Extensions *>(new MPRIS2(*this));
#endif
    return nullptr;
}

Extensions::SettingsWidget *Extensions::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(Extensions)

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
    MPRIS2B = new QCheckBox(tr("Use the program via MPRIS2 interface"));
    MPRIS2B->setChecked(sets().getBool("MPRIS2/Enabled"));
#endif

#ifdef USE_YOUTUBE
    QGroupBox *youTubeB = new QGroupBox("YouTube");

    userNameB = new QCheckBox(tr("Show user name in search results"));
    userNameB->setChecked(sets().getBool("YouTube/ShowUserName"));

    subtitlesB = new QCheckBox(tr("Display available subtitles"));
    subtitlesB->setToolTip(tr("Display subtitles from YouTube. Follow default subtitles language and QMPlay2 language."));
    subtitlesB->setChecked(sets().getBool("YouTube/Subtitles"));

    int idx;

    m_preferredCodec = new QComboBox;
    m_preferredCodec->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    m_preferredCodec->addItems({"VP9", "H.264"});
    idx = m_preferredCodec->findText(sets().getString("YouTube/PreferredCodec"));
    m_preferredCodec->setCurrentIndex(idx > -1 ? idx : 0);

    qualityPreset = new QComboBox;
    qualityPreset->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    qualityPreset->addItems(YouTube::getQualityPresets());
    idx = qualityPreset->findText(sets().getString("YouTube/QualityPreset"));
    qualityPreset->setCurrentIndex(idx > -1 ? idx : 3);

    layout = new QGridLayout(youTubeB);
    layout->addWidget(userNameB, 0, 0, 1, 2);
    layout->addWidget(subtitlesB, 1, 0, 1, 2);
    layout->addWidget(new QLabel(tr("Preferred video codec") + ": "), 2, 0, 1, 1);
    layout->addWidget(m_preferredCodec, 2, 1, 1, 1);
    layout->addWidget(new QLabel(tr("Preferred quality") + ": "), 3, 0, 1, 1);
    layout->addWidget(qualityPreset, 3, 1, 1, 1);
    layout->setMargin(2);
#endif

#ifdef USE_LASTFM
    QGroupBox *lastFMB = new QGroupBox("LastFM");

    downloadCoversGB = new QGroupBox(tr("Download covers"));
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
    mainLayout->setProperty("NoVHSpacer", false);
#ifdef USE_MPRIS2
    mainLayout->addWidget(MPRIS2B);
#endif
#ifdef USE_YOUTUBE
    mainLayout->addWidget(youTubeB);
#endif
#ifdef USE_LASTFM
    mainLayout->addWidget(lastFMB);
#endif
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

#ifdef USE_YOUTUBE
    sets().set("YouTube/ShowUserName", userNameB->isChecked());
    sets().set("YouTube/Subtitles", subtitlesB->isChecked());
    sets().set("YouTube/QualityPreset", qualityPreset->currentText());
    sets().set("YouTube/PreferredCodec", m_preferredCodec->currentText());
#endif

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
