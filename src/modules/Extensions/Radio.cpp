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

#include <Radio.hpp>
#include <Version.hpp>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressBar>
#include <QInputDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QAction>
#include <QLabel>

Radio::Radio(Module &module) :
	once(false), net(NULL),
	qmp2Icon(QMPlay2Core.getQMPlay2Pixmap()),
	wlasneStacje(tr("Own radio stations"))
{
	SetModule(module);

	setContextMenuPolicy(Qt::CustomContextMenu);
	popupMenu.addAction(tr("Remove the radio station"), this, SLOT(removeStation()));

	dw = new DockWidget;
	dw->setWindowTitle(tr("Internet radios"));
	dw->setObjectName(RadioName);
	dw->setWidget(this);

	lW = new QListWidget;
	connect(lW, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(openLink()));
	lW->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	lW->setResizeMode(QListView::Adjust);
	lW->setWrapping(true);
	lW->setIconSize(QSize(32, 32));

	QAction *act = new QAction(lW);
	act->setShortcuts(QList<QKeySequence>() << QKeySequence("Return") << QKeySequence("Enter"));
	connect(act, SIGNAL(triggered()), this, SLOT(openLink()));
	act->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	lW->addAction(act);

	infoL = new QLabel;

	progressB = new QProgressBar;

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(lW);
	layout->addWidget(infoL);
	layout->addWidget(progressB);

	progressB->hide();

	connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityChanged(bool)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popup(const QPoint &)));

	addGroup(wlasneStacje);
	nowaStacjaLWI = new QListWidgetItem("-- " + tr("Add new radio station") + " --", lW);
	nowaStacjaLWI->setData(Qt::TextAlignmentRole, Qt::AlignCenter);

	Settings sets("Radio");
	foreach (const QString &entry, sets.get("Radia").toStringList())
	{
		const QStringList nazwa_i_adres = entry.split('\n');
		if (nazwa_i_adres.count() == 2)
			addStation(nazwa_i_adres[0], nazwa_i_adres[1], wlasneStacje);
	}
}
Radio::~Radio()
{
	QStringList radia;
	foreach (QListWidgetItem *lWI, lW->findItems(QString(), Qt::MatchContains))
		if (lWI->data(Qt::ToolTipRole).toString() == wlasneStacje)
			radia += lWI->text() + '\n' + lWI->data(Qt::UserRole).toString();
	Settings sets("Radio");
	sets.set("Radia", radia);
}

DockWidget *Radio::getDockWidget()
{
	return dw;
}

void Radio::visibilityChanged(bool v)
{
	if (v && !once)
	{
		once = true;
		infoL->setText(tr("Downloading list, please wait..."));
		progressB->setMaximum(0);
		progressB->show();

		net = new QNetworkAccessManager(this);
		QNetworkRequest request(QUrl("https://raw.githubusercontent.com/zaps166/QMPlay2OnlineContents/master/RadioList"));
		request.setRawHeader("User-Agent", QMPlay2UserAgent);
		QNetworkReply *netReply = net->get(request);
		connect(netReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64, qint64)));
		connect(netReply, SIGNAL(finished()), this, SLOT(finished()));
	}
}
void Radio::popup(const QPoint &p)
{
	QListWidgetItem *lWI = lW->currentItem();
	if (lWI && lWI->data(Qt::ToolTipRole).toString() == wlasneStacje)
		popupMenu.popup(mapToGlobal(p));
}
void Radio::removeStation()
{
	delete lW->takeItem(lW->currentRow());
}

void Radio::openLink()
{
	QListWidgetItem *lWI = lW->currentItem();
	if (lWI)
	{
		if (lWI == nowaStacjaLWI)
		{
			const QString newStation = tr("Adding a new radio station");
			QString nazwa, adres;
			bool ok;
			nazwa = QInputDialog::getText(this, newStation, tr("Name"), QLineEdit::Normal, QString(), &ok);
			if (ok && !nazwa.isEmpty())
			{
				adres = QInputDialog::getText(this, newStation, tr("Address"), QLineEdit::Normal, "http://", &ok);
				if (ok && !adres.isEmpty() && adres != "http://")
					addStation(nazwa, adres, wlasneStacje);
			}
		}
		else
		{
			const QString url = lWI->data(Qt::UserRole).toString();
			if (!url.isEmpty())
				emit QMPlay2Core.processParam("open", url);
		}
	}
}

void Radio::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (bytesTotal > 0 && bytesTotal != progressB->maximum())
		progressB->setMaximum(bytesTotal);
	progressB->setValue(bytesReceived);
}
void Radio::finished()
{
	QNetworkReply *netReply = (QNetworkReply *)sender();
	bool err = false;
	if (!netReply->error())
	{
		QByteArray RadioList = netReply->readAll();
		if (!RadioList.startsWith("NXRL"))
			err = true;
		else
		{
			int separators = 0;
			QString GroupName;
			while (RadioList.size())
			{
				if (RadioList.startsWith("NXRL"))
				{
					RadioList.remove(0, 4);
					GroupName = RadioList.data();
					RadioList.remove(0, GroupName.toUtf8().length() + 1);
					addGroup(GroupName);
					++separators;
				}

				QString nazwa = RadioList.data();
				RadioList.remove(0, nazwa.toUtf8().length() + 1);
				QString URL = RadioList.data();
				RadioList.remove(0, URL.toUtf8().length() + 1);
				int imgSize = *(const int *)RadioList.constData();
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
				quint8 *imgSizeArr = (quint8 *)&imgSize;
				imgSize = imgSizeArr[0] << 24 | imgSizeArr[1] << 16 | imgSizeArr[2] << 8 | imgSizeArr[3];
#endif
				RadioList.remove(0, sizeof(int));
				QByteArray img = RadioList.mid(0, imgSize);
				RadioList.remove(0, imgSize);
				addStation(nazwa, URL, GroupName, img);
			}

			infoL->setText(tr("Number of radio stations") + ": " + QString::number(lW->count() - separators));
		}
	}
	else
		err = true;
	if (err)
	{
		infoL->setText(tr("Error while downloading list"));
		progressB->hide();
		once = false;
	}
	else
	{
		progressB->deleteLater();
		progressB = NULL;
	}
	netReply->deleteLater();
	net->deleteLater();
	net = NULL;
}

void Radio::addGroup(const QString &groupName)
{
	QFont groupFont;
	groupFont.setBold(true);
	groupFont.setPointSize(groupFont.pointSize() + 2);

	QListWidgetItem *lWI = new QListWidgetItem("\n-- " + groupName + " --\n", lW);
	lWI->setData(Qt::TextAlignmentRole, Qt::AlignCenter);
	lWI->setIcon(QIcon(":/radio"));
	lWI->setFont(groupFont);
}
void Radio::addStation(const QString &nazwa, const QString &URL, const QString &groupName, const QByteArray &img)
{
	QListWidgetItem *lWI = new QListWidgetItem(nazwa);
	lWI->setData(Qt::UserRole, URL);
	lWI->setData(Qt::ToolTipRole, groupName);
	if (img.isEmpty())
		lWI->setIcon(qmp2Icon);
	else
	{
		QPixmap pix;
		pix.loadFromData(img);
		lWI->setIcon(pix);
	}
	if (groupName == wlasneStacje)
		lW->insertItem(lW->row(nowaStacjaLWI), lWI);
	else
		lW->addItem(lWI);
}
