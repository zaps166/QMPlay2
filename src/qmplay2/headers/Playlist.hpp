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
			length(-1.0),
			selected(false),
			queue(0), GID(0), parent(0)
		{}

		QString name, url;
		double length;
		bool selected;
		qint32 queue, GID, parent;
	};
	typedef QList<Entry> Entries;

	enum OpenMode
	{
		NoOpen,
		ReadOnly,
		WriteOnly
	};

	static Entries read(const QString &url, QString *name = NULL);
	static bool write(const Entries &list, const QString &url, QString *name = NULL);
	static QString name(const QString &url);
	static QStringList extensions();

	virtual Entries read() = 0;
	virtual bool write(const Entries &) = 0;

	virtual ~Playlist();
private:
	static Playlist *create(const QString &url, OpenMode openMode, QString *name = NULL);
protected:
	QList<QByteArray> readLines();

	IOController<> ioCtrl;
};

#endif
