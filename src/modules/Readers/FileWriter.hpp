#include <Writer.hpp>

#include <QFile>

class FileWriter : public Writer
{
	bool readyWrite() const;

	qint64 write( const QByteArray & );

	qint64 size() const;
	QString name() const;

	~FileWriter();

	bool open();

	/**/

	mutable QFile f;
};

#define FileWriterName "File Writer"
