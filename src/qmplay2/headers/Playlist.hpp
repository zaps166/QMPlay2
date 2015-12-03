#ifndef PLAYLIST_HPP
#define PLAYLIST_HPP

#include <IOController.hpp>

#include <QString>
#include <QList>

class Playlist
{
public:
	class Entry
	{
	public:
		inline Entry() :
			selected( false ),
			length( -1 ), queue( 0 ), GID( 0 ), parent( 0 )
		{}

		QString name, url;
		bool selected;
		int length, queue, GID, parent;
	};
	typedef QList< Entry > Entries;

	enum OpenMode { NoOpen, ReadOnly, WriteOnly };

	static Entries read( const QString &, QString *name = NULL );
	static bool write( const Entries &, const QString &, QString *name = NULL );
	static QString name( const QString & );
	static QStringList extensions();

	virtual Entries _read() = 0;
	virtual bool _write( const Entries & ) = 0;

	virtual ~Playlist() {}
private:
	static Playlist *create( const QString &, OpenMode, QString *name = NULL );
protected:
	QList< QByteArray > readLines();

	IOController<> ioCtrl;
};

#endif
