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

/**/

class ResultsPleer : public QTreeWidget
{
	Q_OBJECT
public:
	ResultsPleer();
private:
	QMenu menu;
private slots:
	void enqueue();
	void playCurrentEntry();
	void openPage();
	void copyPageURL();

	void playEntry(QTreeWidgetItem *tWI);

	void contextMenu(const QPoint &p);
};

/**/

class QProgressBar;
class QToolButton;
class QCompleter;
class LineEdit;

class ProstoPleerW : public QWidget
{
	friend class ProstoPleer;
	Q_OBJECT
public:
	ProstoPleerW();
private slots:
	void next();

	void searchTextEdited(const QString &text);
	void search();

	void netFinished(QNetworkReply *reply);

	void searchMenu();
private:
	DockWidget *dw;

	LineEdit *searchE;
	QToolButton *searchB, *nextPageB;
	QProgressBar *progressB;
	ResultsPleer *resultsW;

	QCompleter *completer;
	QString lastName;
	int currPage;

	QNetworkReply *autocompleteReply, *searchReply;
	QNetworkAccessManager net;
};

/**/

class ProstoPleer : public QMPlay2Extensions
{
public:
	ProstoPleer(Module &module);

	bool set();

	DockWidget *getDockWidget();

	QList<AddressPrefix> addressPrefixList(bool);
	void convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl);

	QAction *getAction(const QString &, double, const QString &, const QString &, const QString &);
private:
	ProstoPleerW w;
};

#define ProstoPleerName "Prostopleer"
