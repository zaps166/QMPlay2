/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <IPC.hpp>

#include <QSocketNotifier>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/un.h>

static sockaddr_un getSockAddr(const QString &fileName)
{
    sockaddr_un sockAddr;
    sockAddr.sun_family = AF_UNIX;
    strncpy(sockAddr.sun_path, fileName.toLocal8Bit(), sizeof sockAddr.sun_path - 1);
#ifdef Q_OS_MACOS
    sockAddr.sun_len = SUN_LEN(&sockAddr);
#endif
    return sockAddr;
}

/**/

class IPCSocketPriv
{
public:
    inline IPCSocketPriv(const QString &fileName, int fd = -1) :
        fileName(fileName),
        socketNotifier(nullptr),
        fd(fd)
    {}

    QString fileName;
    QSocketNotifier *socketNotifier;
    int fd;
};

class IPCServerPriv
{
public:
    inline IPCServerPriv(const QString &fileName) :
        fileName(fileName),
        socketNotifier(nullptr),
        fd(-1)
    {}

    QString fileName;
    QSocketNotifier *socketNotifier;
    int fd;
};

/**/

IPCSocket::IPCSocket(const QString &fileName, QObject *parent) :
    QIODevice(parent),
    m_priv(new IPCSocketPriv(fileName))
{}
IPCSocket::~IPCSocket()
{
    close();
    delete m_priv;
}

bool IPCSocket::isConnected() const
{
    return m_priv->fd > 0;
}

bool IPCSocket::open(QIODevice::OpenMode mode)
{
    if (!m_priv->fileName.isEmpty())
    {
        const sockaddr_un sockAddr = getSockAddr(m_priv->fileName);
        m_priv->fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_priv->fd > 0 && ::connect(m_priv->fd, (const sockaddr *)&sockAddr, sizeof sockAddr) != 0)
        {
            ::close(m_priv->fd);
            m_priv->fd = -1;
        }
    }

    if (m_priv->fd > 0)
    {
        const unsigned long on = 1;
        ioctl(m_priv->fd, FIONBIO, &on);
        m_priv->socketNotifier = new QSocketNotifier(m_priv->fd, QSocketNotifier::Read, this);
        connect(m_priv->socketNotifier, SIGNAL(activated(int)), this, SLOT(socketReadActive()));
        return QIODevice::open(mode);
    }

    return false;
}
void IPCSocket::close()
{
    if (m_priv->fd > 0)
    {
        delete m_priv->socketNotifier;
        m_priv->socketNotifier = nullptr;

        ::close(m_priv->fd);
        m_priv->fd = -1;
    }
}

void IPCSocket::socketReadActive()
{
    char c;
    m_priv->socketNotifier->setEnabled(false);
    if (::recv(m_priv->fd, &c, 1, MSG_PEEK) == 1)
        emit readyRead();
    else
    {
        emit aboutToClose();
        close();
    }
}

IPCSocket::IPCSocket(int socket, QObject *parent) :
    QIODevice(parent),
    m_priv(new IPCSocketPriv(QString(), socket))
{}

qint64 IPCSocket::readData(char *data, qint64 maxSize)
{
    if (maxSize >= 0 && isOpen())
    {
        if (maxSize == 0)
            return 0;
        const int ret = ::read(m_priv->fd, data, maxSize);
        if (ret > 0)
        {
            m_priv->socketNotifier->setEnabled(true);
            return ret;
        }
    }
    return -1;
}
qint64 IPCSocket::writeData(const char *data, qint64 maxSize)
{
    if (isOpen())
    {
        const int ret = ::write(m_priv->fd, data, maxSize);
        if (ret >= 0)
            return ret;
    }
    return -1;
}

/**/

IPCServer::IPCServer(const QString &fileName, QObject *parent) :
    QObject(parent),
    m_priv(new IPCServerPriv(fileName))
{}
IPCServer::~IPCServer()
{
    close();
    delete m_priv;
}

bool IPCServer::listen()
{
    if (m_priv->fd > 0)
        return true;

    m_priv->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_priv->fd > 0)
    {
        const sockaddr_un sockAddr = getSockAddr(m_priv->fileName);

        if (bind(m_priv->fd, (const sockaddr *)&sockAddr, sizeof sockAddr) == 0 && ::listen(m_priv->fd, 1) == 0)
        {
            m_priv->socketNotifier = new QSocketNotifier(m_priv->fd, QSocketNotifier::Read, this);
            connect(m_priv->socketNotifier, SIGNAL(activated(int)), this, SLOT(socketAcceptActive()));
            return true;
        }

        close();
    }

    return false;
}

void IPCServer::close()
{
    if (m_priv->fd > 0)
    {
        delete m_priv->socketNotifier;

        ::close(m_priv->fd);
        m_priv->fd = -1;

        if (m_priv->socketNotifier)
        {
            unlink(m_priv->fileName.toLocal8Bit());
            m_priv->socketNotifier = nullptr;
        }
    }
}

void IPCServer::socketAcceptActive()
{
    const int client = accept(m_priv->fd, nullptr, nullptr);
    if (client > 0)
    {
        IPCSocket *socket = new IPCSocket(client, this);
        if (socket->open(IPCSocket::ReadWrite))
            emit newConnection(socket);
        else
            socket->deleteLater();
    }
}
