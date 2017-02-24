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

#include <MusicBrowser.hpp>

#include <MusicBrowser/MusicBrowserInterface.hpp>
#include <Functions.hpp>
#include <LineEdit.hpp>

#ifdef USE_PROSTOPLEER
	#include <MusicBrowser/ProstoPleer.hpp>
#endif
#ifdef USE_SOUNDCLOUD
	#include <MusicBrowser/SoundCloud.hpp>
#endif

#include <QStringListModel>
#include <QDesktopServices>
#include <QApplication>
#include <QProgressBar>
#include <QGridLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QCompleter>
#include <QClipboard>
#include <QComboBox>
#include <QMimeData>
#include <QUrl>

MusicBrowserResults::MusicBrowserResults(MusicBrowserInterface *&musicBrowser) :
	m_musicBrowser(musicBrowser)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setIconSize(QSize(22, 22));
	setSortingEnabled(true);
	setIndentation(0);

	header()->setStretchLastSection(false);
	headerItem()->setHidden(true);

	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
	setContextMenuPolicy(Qt::CustomContextMenu);
}
MusicBrowserResults::~MusicBrowserResults()
{}

void MusicBrowserResults::enqueue()
{
	if (m_musicBrowser)
	{
		if (QTreeWidgetItem *tWI = currentItem())
			emit QMPlay2Core.processParam("enqueue", m_musicBrowser->getQMPlay2Url(tWI->data(0, Qt::UserRole).toString()));
	}
}
void MusicBrowserResults::playCurrentEntry()
{
	playEntry(currentItem());
}
void MusicBrowserResults::openPage()
{
	if (m_musicBrowser && m_musicBrowser->hasWebpage())
	{
		if (QTreeWidgetItem *tWI = currentItem())
			QDesktopServices::openUrl(m_musicBrowser->getWebpageUrl(tWI->data(0, Qt::UserRole).toString()));
	}
}
void MusicBrowserResults::copyPageURL()
{
	if (m_musicBrowser && m_musicBrowser->hasWebpage())
	{
		if (QTreeWidgetItem *tWI = currentItem())
		{
			QMimeData *mimeData = new QMimeData;
			mimeData->setText(m_musicBrowser->getWebpageUrl(tWI->data(0, Qt::UserRole).toString()));
			QApplication::clipboard()->setMimeData(mimeData);
		}
	}
}

void MusicBrowserResults::playEntry(QTreeWidgetItem *tWI)
{
	if (tWI)
		emit QMPlay2Core.processParam("open", m_musicBrowser->getQMPlay2Url(tWI->data(0, Qt::UserRole).toString()));
}

void MusicBrowserResults::contextMenu(const QPoint &point)
{
	m_menu.clear();
	if (!m_musicBrowser)
		return;
	if (QTreeWidgetItem *tWI = currentItem())
	{
		m_menu.addAction(tr("Enqueue"), this, SLOT(enqueue()));
		m_menu.addAction(tr("Play"), this, SLOT(playCurrentEntry()));
		m_menu.addSeparator();
		if (m_musicBrowser->hasWebpage())
		{
			m_menu.addAction(tr("Open the page in the browser"), this, SLOT(openPage()));
			m_menu.addAction(tr("Copy page address"), this, SLOT(copyPageURL()));
			m_menu.addSeparator();
		}
		const QString name = tWI->text(0);
		for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
		{
			QString addressPrefixName, url, param;
			if (Functions::splitPrefixAndUrlIfHasPluginPrefix(m_musicBrowser->getQMPlay2Url(tWI->data(0, Qt::UserRole).toString()), &addressPrefixName, &url, &param))
			{
				const bool self = dynamic_cast<MusicBrowser *>(QMPlay2Ext);
				for (QAction *act : QMPlay2Ext->getActions(name, -2, url, addressPrefixName, param))
				{
					if (!self || act->property("ptr").value<quintptr>() != (quintptr)m_musicBrowser)
					{
						act->setParent(&m_menu);
						m_menu.addAction(act);
					}
				}
			}
		}
		m_menu.popup(viewport()->mapToGlobal(point));
	}
}

/**/

MusicBrowser::MusicBrowser(Module &module) :
	m_musicBrowser(nullptr),
	m_completer(new QCompleter(new QStringListModel(this), this)),
	m_currPage(1),
	m_autocompleteReply(nullptr), m_searchReply(nullptr),
	m_net(this)
{
	SetModule(module);

#ifdef USE_PROSTOPLEER
	m_musicBrowsers.emplace_back(new ProstoPleer);
#endif
#ifdef USE_SOUNDCLOUD
	m_musicBrowsers.emplace_back(new SoundCloud);
#endif

	m_dW = new DockWidget;
	connect(m_dW, SIGNAL(visibilityChanged(bool)), this, SLOT(setEnabled(bool)));
	m_dW->setWindowTitle(MusicBrowserName);
	m_dW->setObjectName(MusicBrowserName);
	m_dW->setWidget(this);

	m_completer->setCaseSensitivity(Qt::CaseInsensitive);

	m_searchE = new LineEdit;
	connect(m_searchE, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextEdited(const QString &)));
	connect(m_searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
	connect(m_searchE, SIGNAL(returnPressed()), this, SLOT(search()));
	m_searchE->setCompleter(m_completer);

	m_providersB = new QComboBox;
	connect(m_providersB, SIGNAL(currentIndexChanged(int)), this, SLOT(providerChanged(int)));

	m_searchB = new QToolButton;
	connect(m_searchB, SIGNAL(clicked()), this, SLOT(search()));
	m_searchB->setIcon(QMPlay2Core.getIconFromTheme("edit-find"));
	m_searchB->setToolTip(tr("Search"));
	m_searchB->setAutoRaise(true);

	m_nextPageB = new QToolButton;
	connect(m_nextPageB, SIGNAL(clicked()), this, SLOT(next()));
	m_nextPageB->setAutoRaise(true);
	m_nextPageB->setArrowType(Qt::RightArrow);
	m_nextPageB->setToolTip(tr("Next page"));
	m_nextPageB->hide();

	m_progressB = new QProgressBar;
	m_progressB->setRange(0, 0);
	m_progressB->hide();

	m_resultsW = new MusicBrowserResults(m_musicBrowser);

	connect(&m_net, SIGNAL(finished(NetworkReply *)), this, SLOT(netFinished(NetworkReply *)));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(m_providersB, 0, 0, 1, 1);
	layout->addWidget(m_searchE, 0, 1, 1, 1);
	layout->addWidget(m_searchB, 0, 2, 1, 1);
	layout->addWidget(m_nextPageB, 0, 3, 1, 1);
	layout->addWidget(m_resultsW, 1, 0, 1, 4);
	layout->addWidget(m_progressB, 2, 0, 1, 4);
	setLayout(layout);

	for (const auto &m : m_musicBrowsers)
		m_providersB->addItem(m->icon(), m->name());
}
MusicBrowser::~MusicBrowser()
{}

DockWidget *MusicBrowser::getDockWidget()
{
	return m_dW;
}

QList<QMPlay2Extensions::AddressPrefix> MusicBrowser::addressPrefixList(bool img) const
{
	QList<AddressPrefix> ret;
	for (const auto &m : m_musicBrowsers)
		ret.append(m->addressPrefixList(img));
	return ret;
}
void MusicBrowser::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	Q_UNUSED(param)
	if (stream_url || img)
		for (const auto &m : m_musicBrowsers)
			if (m->convertAddress(prefix, url, stream_url, name, img, extension, ioCtrl))
				break;
}

QVector<QAction *> MusicBrowser::getActions(const QString &name, double, const QString &url, const QString &, const QString &)
{
	QVector<QAction *> actions;
	if (name != url)
	{
		for (size_t i = 0; i < m_musicBrowsers.size(); ++i)
		{
			MusicBrowserInterface *m = m_musicBrowsers[i].get();
			QAction *act = m->getAction();
			act->connect(act, SIGNAL(triggered()), this, SLOT(searchMenu()));
			act->setProperty("ptr", (quintptr)m);
			act->setProperty("idx", (quint32)i);
			act->setProperty("name", name);
			actions.append(act);
		}
	}
	return actions;
}

void MusicBrowser::providerChanged(int idx)
{
	m_searchE->clearText();

	m_musicBrowser = m_musicBrowsers[idx].get();

	m_resultsW->headerItem()->setHidden(false);
	m_musicBrowser->prepareWidget(m_resultsW);
}

void MusicBrowser::next()
{
	++m_currPage;
	search();
}

void MusicBrowser::searchTextEdited(const QString &text)
{
	if (m_autocompleteReply)
	{
		m_autocompleteReply->deleteLater();
		m_autocompleteReply = nullptr;
	}
	if (text.isEmpty())
		((QStringListModel *)m_completer->model())->setStringList({});
	else if (m_musicBrowser)
		m_autocompleteReply = m_musicBrowser->getCompleterReply(text, m_net);
}
void MusicBrowser::search()
{
	const QString name = m_searchE->text();
	if (m_autocompleteReply)
	{
		m_autocompleteReply->deleteLater();
		m_autocompleteReply = nullptr;
	}
	if (m_searchReply)
	{
		m_searchReply->deleteLater();
		m_searchReply = nullptr;
	}
	m_resultsW->clear();
	if (!name.isEmpty())
	{
		if (m_lastName != name || sender() == m_searchE || sender() == m_searchB)
			m_currPage = 1;
		if (m_musicBrowser)
		{
			m_searchReply = m_musicBrowser->getSearchReply(name, m_currPage, m_net);
			m_progressB->show();
		}
	}
	else
	{
		m_nextPageB->hide();
		m_progressB->hide();
	}
	m_lastName = name;
}

void MusicBrowser::netFinished(NetworkReply *reply)
{
	if (reply->hasError())
	{
		if (reply == m_searchReply)
		{
			m_lastName.clear();
			m_nextPageB->hide();
			m_progressB->hide();
			emit QMPlay2Core.sendMessage(tr("Connection error"), MusicBrowserName, 3);
		}
	}
	else
	{
		const QByteArray replyData = reply->readAll();
		if (reply == m_autocompleteReply)
		{
			if (m_musicBrowser)
			{
				const QStringList suggestions = m_musicBrowser->getCompletions(replyData);
				if (!suggestions.isEmpty())
				{
					((QStringListModel *)m_completer->model())->setStringList(suggestions);
					if (m_searchE->hasFocus())
						m_completer->complete();
				}
			}
		}
		else if (reply == m_searchReply)
		{
			if (m_musicBrowser)
				m_musicBrowser->addSearchResults(replyData, m_resultsW);
			m_nextPageB->setVisible(m_resultsW->topLevelItemCount());
		}
	}
	if (reply == m_autocompleteReply)
		m_autocompleteReply = nullptr;
	else if (reply == m_searchReply)
	{
		m_searchReply = nullptr;
		m_progressB->hide();
	}
	reply->deleteLater();
}

void MusicBrowser::searchMenu()
{
	const QString name = sender()->property("name").toString();
	if (!name.isEmpty())
	{
		m_providersB->setCurrentIndex(sender()->property("idx").toUInt());
		if (!m_dW->isVisible())
			m_dW->show();
		m_dW->raise();
		m_searchE->setText(name);
		search();
	}
}
