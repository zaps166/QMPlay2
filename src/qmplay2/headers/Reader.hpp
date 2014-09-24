#ifndef READER_HPP
#define READER_HPP

#ifndef SEEK_SET
	#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
	#define SEEK_CUR 1
#endif
#ifndef SEEK_END
	#define SEEK_END 2
#endif

#include <ModuleCommon.hpp>
#include <ModuleParams.hpp>

class Reader : public ModuleCommon, public ModuleParams
{
public:
	static bool create( const QString &, Reader *&, const bool &br = false, QMutex *readerMutex = NULL, const QString &plugName = QString() );

	inline QString url() const
	{
		return _url;
	}

	virtual bool readyRead() const = 0;
	virtual bool canSeek() const = 0;

	virtual bool seek( qint64, int wh = SEEK_SET ) = 0;
	virtual QByteArray read( qint64 ) = 0;
	virtual QByteArray readLine();
	virtual void pause() {}
	virtual bool atEnd() const = 0;
	virtual void abort() {} //must be thread-safe!

	virtual qint64 size() const = 0;
	virtual qint64 pos() const = 0;
	virtual QString name() const = 0;

	virtual ~Reader() {}
private:
	virtual bool open() = 0;
protected:
	QString _url;
};

#endif
