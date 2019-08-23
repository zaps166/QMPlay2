/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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
#ifdef USE_LYRICS
    #include <Lyrics.hpp>
#endif
#ifdef USE_MEDIABROWSER
    #include <MediaBrowser.hpp>
#endif

#include <QCoreApplication>

Extensions::Extensions() :
    Module("Extensions"),
    downloader(":/downloader.svgz"), youtube(":/youtube.svgz"), radio(":/radio.svgz")
{
    m_icon = QIcon(":/Extensions.svgz");

#ifdef USE_LASTFM
    lastfm = QIcon(":/lastfm.svgz");
#endif

    init("YouTube/ShowUserName", false);
    init("YouTube/Subtitles", true);
    init("YouTube/SortBy", 0);

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
    else if (name == YouTubeName)
        return static_cast<QMPlay2Extensions *>(new YouTube(*this));
#ifdef USE_LASTFM
    else if (name == LastFMName)
        return static_cast<QMPlay2Extensions *>(new LastFM(*this));
#endif
    else if (name == RadioName)
        return static_cast<QMPlay2Extensions *>(new Radio(*this));
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
    MPRIS2B = new QCheckBox(tr("Using the program via MPRIS2 interface"));
    MPRIS2B->setChecked(sets().getBool("MPRIS2/Enabled"));
#endif

    QGroupBox *youTubeB = new QGroupBox("YouTube");

    userNameB = new QCheckBox(tr("Show user name in search results"));
    userNameB->setChecked(sets().getBool("YouTube/ShowUserName"));

    subtitlesB = new QCheckBox(tr("Display subtitles if available"));
    subtitlesB->setToolTip(tr("Displays subtitles from YouTube. Follows default subtitles language and QMPlay2 language."));
    subtitlesB->setChecked(sets().getBool("YouTube/Subtitles"));

    layout = new QGridLayout(youTubeB);
    layout->addWidget(userNameB);
    layout->addWidget(subtitlesB);
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
    mainLayout->setProperty("NoVHSpacer", false);
#ifdef USE_MPRIS2
    mainLayout->addWidget(MPRIS2B);
#endif
    mainLayout->addWidget(youTubeB);
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

    sets().set("YouTube/ShowUserName", userNameB->isChecked());
    sets().set("YouTube/Subtitles", subtitlesB->isChecked());

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
