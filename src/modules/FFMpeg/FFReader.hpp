#ifndef FFREADER_HPP
#define FFREADER_HPP

#include <Reader.hpp>

struct AVIOContext;

class FFReader : public Reader
{
public:
	FFReader( Module &module );
private:
	bool readyRead() const;
	bool canSeek() const;

	bool seek( qint64, int wh );
	QByteArray read( qint64 );
	void pause();
	bool atEnd() const;
	void abort();

	qint64 size() const;
	qint64 pos() const;
	QString name() const;

	bool open();

	/**/

	~FFReader();

	AVIOContext *avioCtx;
	bool aborted, paused, canRead;
};

#endif // FFREADER_HPP
