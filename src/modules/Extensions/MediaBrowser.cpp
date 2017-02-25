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

#include <MediaBrowser.hpp>

#include <MediaBrowser/Common.hpp>
#include <Functions.hpp>
#include <LineEdit.hpp>

#ifdef USE_PROSTOPLEER
	#include <MediaBrowser/ProstoPleer.hpp>
#endif
#ifdef USE_SOUNDCLOUD
	#include <MediaBrowser/SoundCloud.hpp>
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

MediaBrowserResults::MediaBrowserResults(MediaBrowserCommon *&mediaBrowser) :
	m_mediaBrowser(mediaBrowser)
{
	headerItem()->setHidden(true);

	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(playEntry(QTreeWidgetItem *)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
	setContextMenuPolicy(Qt::CustomContextMenu);
}
MediaBrowserResults::~MediaBrowserResults()
{}

void MediaBrowserResults::enqueue()
{
	if (m_mediaBrowser)
	{
		if (QTreeWidgetItem *tWI = currentItem())
			emit QMPlay2Core.processParam("enqueue", m_mediaBrowser->getQMPlay2Url(tWI->data(0, Qt::UserRole).toString()));
	}
}
void MediaBrowserResults::playCurrentEntry()
{
	playEntry(currentItem());
}
void MediaBrowserResults::openPage()
{
	if (m_mediaBrowser && m_mediaBrowser->hasWebpage())
	{
		if (QTreeWidgetItem *tWI = currentItem())
			QDesktopServices::openUrl(m_mediaBrowser->getWebpageUrl(tWI->data(0, Qt::UserRole).toString()));
	}
}
void MediaBrowserResults::copyPageURL()
{
	if (m_mediaBrowser && m_mediaBrowser->hasWebpage())
	{
		if (QTreeWidgetItem *tWI = currentItem())
		{
			QMimeData *mimeData = new QMimeData;
			mimeData->setText(m_mediaBrowser->getWebpageUrl(tWI->data(0, Qt::UserRole).toString()));
			QApplication::clipboard()->setMimeData(mimeData);
		}
	}
}

void MediaBrowserResults::playEntry(QTreeWidgetItem *tWI)
{
	if (tWI)
		emit QMPlay2Core.processParam("open", m_mediaBrowser->getQMPlay2Url(tWI->data(0, Qt::UserRole).toString()));
}

void MediaBrowserResults::contextMenu(const QPoint &point)
{
	m_menu.clear();
	if (!m_mediaBrowser)
		return;
	if (QTreeWidgetItem *tWI = currentItem())
	{
		m_menu.addAction(tr("Enqueue"), this, SLOT(enqueue()));
		m_menu.addAction(tr("Play"), this, SLOT(playCurrentEntry()));
		m_menu.addSeparator();
		if (m_mediaBrowser->hasWebpage())
		{
			m_menu.addAction(tr("Open the page in the browser"), this, SLOT(openPage()));
			m_menu.addAction(tr("Copy page address"), this, SLOT(copyPageURL()));
			m_menu.addSeparator();
		}
		const QString name = tWI->text(0);
		for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
		{
			QString addressPrefixName, url, param;
			if (Functions::splitPrefixAndUrlIfHasPluginPrefix(m_mediaBrowser->getQMPlay2Url(tWI->data(0, Qt::UserRole).toString()), &addressPrefixName, &url, &param))
			{
				const bool self = dynamic_cast<MediaBrowser *>(QMPlay2Ext);
				for (QAction *act : QMPlay2Ext->getActions(name, -2, url, addressPrefixName, param))
				{
					if (!self || act->property("ptr").value<quintptr>() != (quintptr)m_mediaBrowser)
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

MediaBrowser::MediaBrowser(Module &module) :
	m_mediaBrowser(nullptr),
	m_completer(new QCompleter(new QStringListModel(this), this)),
	m_currPage(1),
	m_autocompleteReply(nullptr), m_searchReply(nullptr),
	m_net(this)
{
#ifdef USE_PROSTOPLEER
	m_mediaBrowsers.emplace_back(new ProstoPleer);
#endif
#ifdef USE_SOUNDCLOUD
	m_mediaBrowsers.emplace_back(new SoundCloud);
#endif

	m_dW = new DockWidget;
	connect(m_dW, SIGNAL(visibilityChanged(bool)), this, SLOT(setEnabled(bool)));
	m_dW->setWindowTitle(MediaBrowserName);
	m_dW->setObjectName(MediaBrowserName);
	m_dW->setWidget(this);

	m_completer->setCaseSensitivity(Qt::CaseInsensitive);

	m_searchE = new LineEdit;
	connect(m_searchE, SIGNAL(textEdited(const QString &)), this, SLOT(searchTextEdited(const QString &)));
	connect(m_searchE, SIGNAL(clearButtonClicked()), this, SLOT(search()));
	connect(m_searchE, SIGNAL(returnPressed()), this, SLOT(search()));
	m_searchE->setCompleter(m_completer);

	m_providersB = new QComboBox;
	for (const auto &m : m_mediaBrowsers)
		m_providersB->addItem(m->icon(), m->name());
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

	m_resultsW = new MediaBrowserResults(m_mediaBrowser);

	connect(&m_net, SIGNAL(finished(NetworkReply *)), this, SLOT(netFinished(NetworkReply *)));

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(m_providersB, 0, 0, 1, 1);
	layout->addWidget(m_searchE, 0, 1, 1, 1);
	layout->addWidget(m_searchB, 0, 2, 1, 1);
	layout->addWidget(m_nextPageB, 0, 3, 1, 1);
	layout->addWidget(m_resultsW, 1, 0, 1, 4);
	layout->addWidget(m_progressB, 2, 0, 1, 4);
	setLayout(layout);

	SetModule(module);
}
MediaBrowser::~MediaBrowser()
{}

bool MediaBrowser::set()
{
	const QString provider = sets().getString("MediaBrowser/Provider");
	m_providersB->setCurrentText(provider);
	providerChanged(m_providersB->currentIndex());
	return true;
}

DockWidget *MediaBrowser::getDockWidget()
{
	return m_dW;
}

QList<QMPlay2Extensions::AddressPrefix> MediaBrowser::addressPrefixList(bool img) const
{
	QList<AddressPrefix> ret;
	for (const auto &m : m_mediaBrowsers)
		ret.append(m->addressPrefixList(img));
	return ret;
}
void MediaBrowser::convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl)
{
	Q_UNUSED(param)
	if (stream_url || img)
		for (const auto &m : m_mediaBrowsers)
			if (m->convertAddress(prefix, url, stream_url, name, img, extension, ioCtrl))
				break;
}

QVector<QAction *> MediaBrowser::getActions(const QString &name, double, const QString &url, const QString &, const QString &)
{
	QVector<QAction *> actions;
	if (name != url)
	{
		for (size_t i = 0; i < m_mediaBrowsers.size(); ++i)
		{
			MediaBrowserCommon *m = m_mediaBrowsers[i].get();
			if (QAction *act = m->getAction())
			{
				act->connect(act, SIGNAL(triggered()), this, SLOT(searchMenu()));
				act->setProperty("ptr", (quintptr)m);
				act->setProperty("idx", (quint32)i);
				act->setProperty("name", name);
				actions.append(act);
			}
		}
	}
	return actions;
}

void MediaBrowser::setCompletions(const QStringList &completions)
{
	if (!completions.isEmpty())
	{
		((QStringListModel *)m_completer->model())->setStringList(completions);
		if (m_searchE->hasFocus())
			m_completer->complete();
	}
}

void MediaBrowser::providerChanged(int idx)
{
	m_searchE->clearText();
	m_mediaBrowser = m_mediaBrowsers[idx].get();
	m_mediaBrowser->prepareWidget(m_resultsW);
	sets().set("MediaBrowser/Provider", m_providersB->currentText());
}

void MediaBrowser::next()
{
	++m_currPage;
	search();
}

void MediaBrowser::searchTextEdited(const QString &text)
{
	if (m_autocompleteReply)
	{
		m_autocompleteReply->deleteLater();
		m_autocompleteReply = nullptr;
	}
	if (text.isEmpty())
		((QStringListModel *)m_completer->model())->setStringList({});
	else if (m_mediaBrowser && m_mediaBrowser->hasCompleter())
	{
		m_autocompleteReply = m_mediaBrowser->getCompleterReply(text, m_net);
		if (!m_autocompleteReply)
			setCompletions(m_mediaBrowser->getCompletions());
	}
}
void MediaBrowser::search()
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
		if (m_mediaBrowser)
			m_searchReply = m_mediaBrowser->getSearchReply(name, m_currPage, m_net);
		if (m_searchReply)
			m_progressB->show();
	}
	else
	{
		m_nextPageB->hide();
		m_progressB->hide();
	}
	m_lastName = name;
}

void MediaBrowser::netFinished(NetworkReply *reply)
{
	if (reply->hasError())
	{
		if (reply == m_searchReply)
		{
			m_lastName.clear();
			m_nextPageB->hide();
			m_progressB->hide();
			emit QMPlay2Core.sendMessage(tr("Connection error"), MediaBrowserName, 3);
		}
	}
	else
	{
		const QByteArray replyData = reply->readAll();
		if (reply == m_autocompleteReply)
		{
			if (m_mediaBrowser)
				setCompletions(m_mediaBrowser->getCompletions(replyData));
		}
		else if (reply == m_searchReply)
		{
			if (m_mediaBrowser)
				m_mediaBrowser->addSearchResults(replyData, m_resultsW);
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

void MediaBrowser::searchMenu()
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
