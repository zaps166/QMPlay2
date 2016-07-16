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

#include <Module.hpp>

class Extensions : public Module
{
public:
	Extensions();
private:
	QList<Info> getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();

	QImage downloader, youtube, radio, lastfm;
#ifdef USE_PROSTOPLEER
	QImage prostopleer;
#endif
};

/**/

class QToolButton;
class QListWidget;
class QGroupBox;
class QCheckBox;
class LineEdit;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_OBJECT
public:
	ModuleSettingsWidget(Module &);
private slots:
	void enableItagLists(bool b);
	void browseYoutubedl();
	void loginPasswordEnable(bool checked);
	void passwordEdited();
private:
	void saveSettings();

#ifdef USE_MPRIS2
	QCheckBox *MPRIS2B;
#endif

	QCheckBox *additionalInfoB, *multiStreamB, *subtitlesB;
	LineEdit *youtubedlE;
	QToolButton *youtubedlBrowseB;
	QListWidget *itagLW, *itagVideoLW, *itagAudioLW;

	QGroupBox *downloadCoversGB;
	QCheckBox *allowBigCovers, *updateNowPlayingAndScrobbleB;
	LineEdit *loginE, *passwordE;
};
