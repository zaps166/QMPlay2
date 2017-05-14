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

#pragma once

#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QPointer>
#include <QVector>

constexpr const char *g_radioBrowserBaseApiUrl = "http://www.radio-browser.info/webservice/json";

class NetworkAccess;
class NetworkReply;
struct Column;

class RadioBrowserModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	RadioBrowserModel(const QWidget *widget);
	~RadioBrowserModel();

	inline int elementHeight() const;

	void searchRadios(const QString &text, const QString &searchBy);

	void loadIcons(const int first, const int last);

	QString getName(const QModelIndex &index) const;
	QUrl getUrl(const QModelIndex &index) const;
	QUrl getEditUrl(const QModelIndex &index) const;
	QUrl getHomePageUrl(const QModelIndex &index) const;

	QModelIndex index(int row, int column, const QModelIndex &parent) const override final;
	QModelIndex parent(const QModelIndex &child) const override final;
	int rowCount(const QModelIndex &parent) const override final;
	int columnCount(const QModelIndex &parent) const override final;
	QVariant data(const QModelIndex &index, int role) const override final;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override final;
	Qt::ItemFlags flags(const QModelIndex &index) const override final;
	void sort(int columnIdx, Qt::SortOrder order = Qt::AscendingOrder) override final;

public slots:
	void setFiltrText(const QString &text = QString());

private slots:
	void replyFinished(NetworkReply *reply);

signals:
	void radiosAdded();
	void searchFinished();

private:
	const QWidget *const m_widget;

	NetworkAccess *m_net;
	QPointer<NetworkReply> m_replySearch;

	QVector<QSharedPointer<Column>> m_rows;
	QVector<QSharedPointer<Column>> m_rowsToDisplay;

	int m_sortColumnIdx;
	Qt::SortOrder m_sortOrder;
};

/* Inline implementation */

int RadioBrowserModel::elementHeight() const
{
	return 48;
}
