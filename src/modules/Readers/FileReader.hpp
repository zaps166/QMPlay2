#include <Reader.hpp>

#include <QFile>

class FileReader : public Reader
{
	bool readyRead() const;
	bool canSeek() const;

	bool seek( qint64, int wh = SEEK_SET );
	QByteArray read( qint64 );
	qint64 read( quint8 *buffer, qint64 maxLen );
	QByteArray readLine();
	bool atEnd() const;

	qint64 size() const;
	qint64 pos() const;
	QString name() const;

	~FileReader();

	bool open();

	/**/

	mutable QFile f;
	qint64 fSize;
};

#define FileReaderName "File Reader"
