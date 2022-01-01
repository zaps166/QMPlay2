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

#include "IPC.hpp"

#include <QThread>

#include <windows.h>

class EventNotifier : public QThread
{
public:
    EventNotifier(HANDLE event, QObject *parent) :
        QThread(parent),
        m_event(event)
    {}

    void setEnabled()
    {
        if (!isRunning())
            start();
    }

private:
    void run() override
    {
        WaitForSingleObject(m_event, INFINITE);
    }

    HANDLE m_event;
};

/**/

class PipeData
{
public:
    void closePipe()
    {
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            //Close the pipe before deleting overlapped and before deleting "eventNotifier" to be sure that event is signaled.
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;

            if (eventNotifier)
            {
                eventNotifier->wait();
                delete eventNotifier;
                eventNotifier = NULL;
            }

            if (ov)
            {
                if (ov->hEvent)
                    CloseHandle(ov->hEvent);
                delete ov;
                ov = NULL;
            }
        }
    }

    EventNotifier *eventNotifier;
    OVERLAPPED *ov;
    HANDLE hPipe;
};

class IPCSocketPriv : public PipeData
{
public:
    inline IPCSocketPriv(const QString &fileName, EventNotifier *eventNotifier = NULL, OVERLAPPED *ov = NULL, HANDLE hPipe = INVALID_HANDLE_VALUE) :
        fileName(fileName),
        bufferPos(-1)
    {
        this->eventNotifier = eventNotifier;
        this->ov = ov;
        this->hPipe = hPipe;
    }

    QString fileName;
    QByteArray buffer;
    int bufferPos;
};

class IPCServerPriv : public PipeData
{
public:
    inline IPCServerPriv(const QString &fileName) :
        fileName(fileName)
    {
        eventNotifier = NULL;
        ov = NULL;
        hPipe = INVALID_HANDLE_VALUE;
    }

    QString fileName;
};

/**/

static bool connectToNewClient(HANDLE hPipe, OVERLAPPED *ov)
{
    if (ConnectNamedPipe(hPipe, ov) == 0)
    {
        switch (GetLastError())
        {
            case ERROR_IO_PENDING:
                ResetEvent(ov->hEvent);
            case ERROR_PIPE_CONNECTED:
                return true;
        }
    }
    return false;
}

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
    return m_priv->hPipe != INVALID_HANDLE_VALUE;
}

bool IPCSocket::open(QIODevice::OpenMode mode)
{
    if (!m_priv->fileName.isEmpty())
    {
        for (;;)
        {
            m_priv->hPipe = CreateFileW((const wchar_t *)m_priv->fileName.utf16(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (m_priv->hPipe != INVALID_HANDLE_VALUE)
                break;
            if (GetLastError() != ERROR_PIPE_BUSY || !WaitNamedPipeW((const wchar_t *)m_priv->fileName.utf16(), 1000))
                break;
        }
    }

    if (m_priv->hPipe != INVALID_HANDLE_VALUE)
    {
        if (m_priv->eventNotifier) //Writing in a socket (open by file name) is blocking-only
        {
            connect(m_priv->eventNotifier, SIGNAL(finished()), this, SLOT(socketReadActive()));
            m_priv->eventNotifier->setEnabled();
        }
        return QIODevice::open(mode);
    }

    return false;
}

void IPCSocket::socketReadActive()
{
    DWORD bytes = 0;
    const bool result = GetOverlappedResult(m_priv->hPipe, m_priv->ov, &bytes, false);
    if (result)
    {
        int pos;
        if (m_priv->bufferPos > -1) //Pending
        {
            pos = m_priv->bufferPos;
            m_priv->bufferPos = -1;
        }
        else //Not pending
        {
            pos = m_priv->buffer.size();
            m_priv->buffer.resize(pos + 1024);
            const bool readOK = ReadFile(m_priv->hPipe, m_priv->buffer.data() + pos, 1024, &bytes, m_priv->ov);
            if (!readOK)
            {
                if (GetLastError() == ERROR_IO_PENDING)
                    m_priv->bufferPos = pos;
                else
                    pos = -1; //Error, e.g. "ERROR_BROKEN_PIPE"
            }
        }
        if (pos > -1)
        {
            if (m_priv->bufferPos <= -1) //Not pending
            {
                m_priv->buffer.resize(pos + bytes);
                if (!m_priv->buffer.isEmpty())
                    emit readyRead();
            }
            m_priv->eventNotifier->setEnabled();
            return;
        }
    }

    emit aboutToClose();
    close();
}

IPCSocket::IPCSocket(const PipeData &pipeData, QObject *parent) :
    QIODevice(parent),
    m_priv(new IPCSocketPriv(QString(), pipeData.eventNotifier, pipeData.ov, pipeData.hPipe))
{
    if (m_priv->eventNotifier->parent() == parent)
        m_priv->eventNotifier->setParent(this);
}

void IPCSocket::close()
{
    m_priv->closePipe();
}

qint64 IPCSocket::readData(char *data, qint64 maxSize)
{
    if (maxSize >= 0 && isOpen())
    {
        if (maxSize == 0)
            return 0;
        const int toRead = qMin<qint64>(maxSize, m_priv->buffer.size());
        memcpy(data, m_priv->buffer.constData(), toRead);
        if (toRead == m_priv->buffer.size())
            m_priv->buffer.clear();
        else
            m_priv->buffer.remove(0, toRead);
        return toRead;
    }
    return -1;
}
qint64 IPCSocket::writeData(const char *data, qint64 maxSize)
{
    if (isOpen())
    {
        DWORD written = 0;
        if (WriteFile(m_priv->hPipe, data, maxSize, &written, NULL))
            return written;
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
    m_priv->hPipe = CreateNamedPipeW
    (
        (const wchar_t *)m_priv->fileName.utf16(),
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        0,
        0,
        0,
        NULL
    );
    if (m_priv->hPipe != INVALID_HANDLE_VALUE)
    {
        m_priv->ov = new OVERLAPPED();
        m_priv->ov->hEvent = CreateEvent(NULL, true, true, NULL);
        if (m_priv->ov->hEvent != NULL && connectToNewClient(m_priv->hPipe, m_priv->ov))
        {
            m_priv->eventNotifier = new EventNotifier(m_priv->ov->hEvent, this);
            connect(m_priv->eventNotifier, SIGNAL(finished()), this, SLOT(socketAcceptActive()));
            m_priv->eventNotifier->setEnabled();
            return true;
        }
        close();
    }
    return false;
}

void IPCServer::close()
{
    m_priv->closePipe();
}

void IPCServer::socketAcceptActive()
{
    bool hasOldPipe = false;
    PipeData oldPipe;

    DWORD bytes = 0;
    disconnect(m_priv->eventNotifier, SIGNAL(finished()), this, SLOT(socketAcceptActive()));
    if (GetOverlappedResult(m_priv->hPipe, m_priv->ov, &bytes, false) && bytes == 0)
    {
        oldPipe = *m_priv;
        hasOldPipe = true;
    }
    else
    {
        close();
    }

    //Listen for new connection
    m_priv->eventNotifier = NULL;
    m_priv->ov = NULL;
    m_priv->hPipe = INVALID_HANDLE_VALUE;
    listen();

    if (hasOldPipe)
    {
        //Accept current connection
        IPCSocket *socket = new IPCSocket(oldPipe, this);
        if (socket->open(IPCSocket::ReadWrite))
            emit newConnection(socket);
        else
            socket->deleteLater();
    }
}
