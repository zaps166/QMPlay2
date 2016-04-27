#ifndef WRITER_HPP
#define WRITER_HPP

#include <ModuleCommon.hpp>
#include <ModuleParams.hpp>
#include <IOController.hpp>

#include <QStringList>

class Writer : protected ModuleCommon, public ModuleParams, public BasicIO
{
public:
	static Writer *create(const QString &, const QStringList &modNames = QStringList());

	inline QString getUrl() const
	{
		return _url;
	}

	virtual bool readyWrite() const = 0;

	virtual qint64 write(const QByteArray &) = 0;

	virtual QString name() const = 0;

	virtual ~Writer();
private:
	virtual bool open() = 0;

	QString _url;
};

#endif
