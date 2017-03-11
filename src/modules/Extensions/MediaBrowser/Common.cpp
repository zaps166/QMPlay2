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

#include <MediaBrowser/Common.hpp>

#include <Functions.hpp>

#include <QHeaderView>
#include <QTreeWidget>

MediaBrowserCommon::MediaBrowserCommon(NetworkAccess &net, const QString &name, const QString &imgPath) :
	m_net(net),
	m_name(name),
	m_img(imgPath)
{}

void MediaBrowserCommon::prepareWidget(QTreeWidget *treeW)
{
	treeW->setEditTriggers(QAbstractItemView::NoEditTriggers);
	treeW->headerItem()->setHidden(false);
	treeW->setSortingEnabled(true);
	treeW->setIconSize({22, 22});
	treeW->setIndentation(0);

	treeW->setColumnCount(1);
	treeW->header()->setStretchLastSection(false);
	Functions::setHeaderSectionResizeMode(treeW->header(), 0, QHeaderView::Stretch);
}

void MediaBrowserCommon::setCompleterListCallback(const CompleterReadyCallback &callback)
{
	Q_UNUSED(callback)
}

QMPlay2Extensions::AddressPrefix MediaBrowserCommon::addressPrefix(bool img) const
{
	return QMPlay2Extensions::AddressPrefix(m_name, img ? m_img : QImage());
}
