#include <Writer.hpp>

#include <QSemaphore>

#include <SLES/OpenSLES.h>

class OpenSLESWriter : public Writer
{
public:
	OpenSLESWriter(Module &);
private:
	~OpenSLESWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	qint64 write(const QByteArray &);
	void pause();

	QString name() const;

	bool open();

	/**/

	void close();

	SLObjectItf engineObject;
	SLEngineItf engineEngine;
	SLObjectItf outputMixObject;
	SLObjectItf bqPlayerObject;
	SLPlayItf bqPlayerPlay;
	SLBufferQueueItf bqPlayerBufferQueue;

	QVector< QVector< qint16 > > buffers;
	QVector< qint16 > tmpBuffer;
	QSemaphore sem;
	int currBuffIdx;

	int sample_rate, channels;
	bool paused;
};

#define OpenSLESWriterName "OpenSL|ES"
