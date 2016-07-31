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

#include <QMPlay2Extensions.hpp>

#include <QNetworkAccessManager>
#include <QTreeWidget>
#include <QMenu>
#include <QMap>

class QNetworkReply;
class QProgressBar;
class QToolButton;
class QCompleter;
class YouTubeDL;
class QSpinBox;
class LineEdit;
class QLabel;

/**/

class ResultsYoutube : public QTreeWidget
{
	Q_OBJECT
public:
	ResultsYoutube();
	inline ~ResultsYoutube()
	{
		removeTmpFile();
	}

	void clearAll();

	QList<int> itags, itagsVideo, itagsAudio;
private:
	QTreeWidgetItem *getDefaultQuality(const QTreeWidgetItem *tWI);

	void removeTmpFile();
	void playOrEnqueue(const QString &param, QTreeWidgetItem *tWI);

	void mouseMoveEvent(QMouseEvent *);

	QString fileToRemove;
	QMenu menu;
private slots:
	void enqueue();
	void playCurrentEntry();
	void playEntry(QTreeWidgetItem *tWI);

	void openPage();
	void copyPageURL();
	void copyStreamURL();

	void contextMenu(const QPoint &p);
};

/**/

class PageSwitcher : public QWidget
{
public:
	PageSwitcher(QWidget *youTubeW);

	QToolButton *prevB, *nextB;
	QSpinBox *currPageB;
};

/**/

class YouTubeW : public QWidget
{
	friend class YouTube;
	Q_OBJECT
public:
	YouTubeW(QWidget *parent = NULL);

	DockWidget *dw;
private slots:
	void showSettings();

	void next();
	void prev();
	void chPage();

	void searchTextEdited(const QString &text);
	void search();

	void netFinished(QNetworkReply *reply);

	void searchMenu();
private:
	void deleteReplies();

	void setAutocomplete(const QByteArray &data);
	void setSearchResults(QString data);

	QStringList getYouTubeVideo(const QString &data, const QString &PARAM = QString(), QTreeWidgetItem *tWI = NULL, const QString &url = QString(), IOController<YouTubeDL> *youtube_dl = NULL); //jeżeli (tWI == NULL) to zwraca {URL, file_extension, TITLE}
	QStringList getUrlByItagPriority(const QList<int> &itags, QStringList ret);

	void preparePlaylist(const QString &data, QTreeWidgetItem *tWI);

	LineEdit *searchE;
	QToolButton *showSettingsB, *searchB;
	ResultsYoutube *resultsW;
	QProgressBar *progressB;
	PageSwitcher *pageSwitcher;

	QString lastTitle;
	const QSize imgSize;
	QCompleter *completer;
	int currPage;

	QNetworkReply *autocompleteReply, *searchReply;
	QList<QNetworkReply *> linkReplies, imageReplies;
	QNetworkAccessManager net;

	QString youtubedl;
	bool multiStream, subtitles;
};

/**/

typedef QPair<QStringList, QList<int> > ItagNames;

class YouTube : public QMPlay2Extensions
{
public:
	YouTube(Module &module);

	enum MediaType {MEDIA_AV, MEDIA_VIDEO, MEDIA_AUDIO};
	static ItagNames getItagNames(const QStringList &itagList, MediaType mediaType);

	bool set();

	DockWidget *getDockWidget();

	QList<AddressPrefix> addressPrefixList(bool) const;
	void convertAddress(const QString &, const QString &, const QString &, QString *, QString *, QImage *, QString *, IOController<> *ioCtrl);

	QAction *getAction(const QString &, double, const QString &, const QString &, const QString &);
private:
	YouTubeW w;
};

#define YouTubeName "YouTube Browser"
