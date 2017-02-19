/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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
#include <Http.hpp>

#include <QTreeWidget>
#include <QMenu>

/**/

class ResultsSound : public QTreeWidget
{
	Q_OBJECT
public:
	ResultsSound();
private:
	QMenu menu;
private slots:
	void enqueue();
	void playCurrentEntry();

	void playEntry(QTreeWidgetItem *tWI);

	void contextMenu(const QPoint &p);
};

/**/

class QProgressBar;
class QToolButton;
class LineEdit;

class SoundCloudW : public QWidget
{
	friend class SoundCloud;
	Q_OBJECT
public:
	SoundCloudW();
private slots:
	void next();

	void search();

	void netFinished(HttpReply *reply);

	void searchMenu();
private:
	DockWidget *dw;

	LineEdit *searchE;
	QToolButton *searchB, *nextPageB;
	QProgressBar *progressB;
	ResultsSound *resultsW;

	QString lastName;
	int currPage;

	HttpReply *searchReply;
	Http net;
};

/**/

class SoundCloud : public QMPlay2Extensions
{
public:
	SoundCloud(Module &module);

	bool set() override;

	DockWidget *getDockWidget() override;

	QList<AddressPrefix> addressPrefixList(bool) const override;
	void convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl) override;

	QAction *getAction(const QString &, double, const QString &, const QString &, const QString &) override;
private:
	SoundCloudW w;
};

#define SoundCloudName "SoundCloud"
