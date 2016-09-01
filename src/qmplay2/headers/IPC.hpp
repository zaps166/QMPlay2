#ifndef IPC_HPP
#define IPC_HPP

#include <QIODevice>
#include <QString>

class IPCSocketPriv;
class IPCServerPriv;
struct PipeData;

class QSocketNotifier;

class IPCSocket : public QIODevice
{
	Q_OBJECT
	friend class IPCServer;
public:
	IPCSocket(const QString &fileName, QObject *parent = NULL);
	~IPCSocket();

	bool isConnected() const;

	bool open(OpenMode mode);
	void close();

private slots:
	void socketReadActive();

private:
#ifdef Q_OS_WIN
	IPCSocket(const PipeData &pipeData, QObject *parent);
#else
	IPCSocket(int socket, QObject *parent);
#endif

	qint64 readData(char *data, qint64 maxSize);
	qint64 writeData(const char *data, qint64 maxSize);

	IPCSocketPriv *m_priv;
};

/**/

class IPCServer : public QObject
{
	Q_OBJECT
public:
	IPCServer(const QString &fileName, QObject *parent = NULL);
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

#endif // IPC_HPP
