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
#include <IOController.hpp>

class Reader : protected ModuleCommon, public ModuleParams, public BasicIO
{
public:
	static bool create(const QString &url, IOController<Reader> &reader, const QString &plugName = QString());

	inline QString getUrl() const
	{
		return _url;
	}

	virtual bool readyRead() const = 0;
	virtual bool canSeek() const = 0;

	virtual bool seek(qint64, int wh = SEEK_SET) = 0;
	virtual QByteArray read(qint64) = 0;
	virtual bool atEnd() const = 0;

	virtual qint64 size() const = 0;
	virtual qint64 pos() const = 0;
	virtual QString name() const = 0;

	virtual ~Reader() {}
private:
	virtual bool open() = 0;

	QString _url;
};

#endif
