/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <QMPlay2Lib.hpp>

#include <QIODevice>
#include <QString>

class IPCSocketPriv;
class IPCServerPriv;
#ifdef Q_OS_WIN
	class PipeData;
#endif

class QSocketNotifier;

class QMPLAY2SHAREDLIB_EXPORT IPCSocket final : public QIODevice
{
	Q_OBJECT
	friend class IPCServer;
public:
	IPCSocket(const QString &fileName, QObject *parent = nullptr);
	~IPCSocket();

	bool isConnected() const;

	bool open(OpenMode mode) override;
	void close() override;

private slots:
	void socketReadActive();

private:
#ifdef Q_OS_WIN
	IPCSocket(const PipeData &pipeData, QObject *parent);
#else
	IPCSocket(int socket, QObject *parent);
#endif

	qint64 readData(char *data, qint64 maxSize) override;
	qint64 writeData(const char *data, qint64 maxSize) override;

	IPCSocketPriv *m_priv;
};

/**/

class QMPLAY2SHAREDLIB_EXPORT IPCServer final : public QObject
{
	Q_OBJECT
public:
	IPCServer(const QString &fileName, QObject *parent = nullptr);
	~IPCServer();

	bool listen();
	void close();

signals:
	void newConnection(IPCSocket *socket);

private slots:
	void socketAcceptActive();

private:
	IPCServerPriv *m_priv;
};
