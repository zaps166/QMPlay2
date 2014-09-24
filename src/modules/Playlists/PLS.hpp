#include <Playlist.hpp>

class PLS : public Playlist
{
	QList< Entry > _read();
	bool _write( const QList< Entry > & );

	~PLS() {}

	void prepareList( class QList< Playlist::Entry > *, int );
};

#define PLSName "PLS"
