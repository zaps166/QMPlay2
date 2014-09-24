#ifndef AVTHREAD_HPP
#define AVTHREAD_HPP

#include <QStringList>
#include <QThread>
#include <QMutex>

class PlayClass;
class Decoder;
class Writer;

class AVThread : public QThread
{
	Q_OBJECT
public:
	void destroyDec();
	inline void setDec( Decoder *_dec )
	{
		dec = _dec;
	}

	bool lock();
	inline void unlock()
	{
		mutex.unlock();
	}

	inline bool isWaiting() const
	{
		return waiting;
	}

	virtual void stop( bool terminate = false );

	Decoder *dec;
	Writer *writer;
protected:
	AVThread( PlayClass &, const QString &, Writer *writer = NULL, const QStringList &pluginsName = QStringList() );
	virtual ~AVThread();

	void terminate();

	PlayClass &playC;

	volatile bool br, br2;
	bool waiting;
	QMutex mutex;
};

#endif //AVTHREAD_HPP
