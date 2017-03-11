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

#include <QMPlay2Extensions.hpp>

#include <functional>

class NetworkAccess;
class NetworkReply;
class QTreeWidget;

class MediaBrowserCommon
{
public:
	using CompleterReadyCallback = std::function<void()>;

	class Description
	{
	public:
		Description() = default;
		inline Description(const QString &descr, NetworkReply *reply) :
			description(descr),
			imageReply(reply)
		{}

		QString description;
		NetworkReply *imageReply = nullptr;
	};

	enum class CompleterMode
	{
		None,
		Continuous,
		All
	};

	MediaBrowserCommon(NetworkAccess &net, const QString &name, const QString &imgPath);
	virtual ~MediaBrowserCommon() = default;


	inline QString name() const;
	inline QIcon icon() const;


	virtual void prepareWidget(QTreeWidget *treeW);


	virtual QString getQMPlay2Url(const QString &text) const = 0;

	virtual NetworkReply *getSearchReply(const QString &text, const qint32 page) = 0;
	virtual Description addSearchResults(const QByteArray &reply, QTreeWidget *treeW) = 0;
	virtual bool hasMultiplePages() const = 0;

	virtual bool hasWebpage() const = 0;
	virtual QString getWebpageUrl(const QString &text) const = 0;

	virtual CompleterMode completerMode() const = 0;
	virtual NetworkReply *getCompleterReply(const QString &text) = 0;
	virtual QStringList getCompletions(const QByteArray &reply = QByteArray()) = 0;
	virtual void setCompleterListCallback(const CompleterReadyCallback &callback);


	virtual QMPlay2Extensions::AddressPrefix addressPrefix(bool img) const = 0;

	virtual QAction *getAction() const = 0;

	virtual bool convertAddress(const QString &prefix, const QString &url, QString *streamUrl, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl) = 0;

signals:
	void gotCompleterList();

protected:
	NetworkAccess &m_net;
	const QString m_name;
	const QImage m_img;
};

/* Inline implementation */

QString MediaBrowserCommon::name() const
{
	return m_name;
}
QIcon MediaBrowserCommon::icon() const
{
	return QPixmap::fromImage(m_img);
}
