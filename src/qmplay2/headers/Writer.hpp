#ifndef WRITER_HPP
#define WRITER_HPP

#include <ModuleCommon.hpp>
#include <ModuleParams.hpp>

#include <QStringList>

class Writer : public ModuleCommon, public ModuleParams
{
public:
	static Writer *create( const QString &, const QStringList &modNames = QStringList() );

	inline QString url() const
	{
		return _url;
	}

	virtual bool readyWrite() const = 0;

	virtual qint64 write( const QByteArray & ) = 0;
	virtual void pause() {}

	virtual qint64 size() const
	{
		return -1;
	}
	virtual QString name() const = 0;

	virtual ~Writer() {}
private:
	virtual bool open() = 0;

	QString _url;
};

#endif
