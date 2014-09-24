#include <Playlist.hpp>

class M3U : public Playlist
{
	QList< Entry > _read();
	bool _write( const QList< Entry > & );

	~M3U() {}
};

#define M3UName "M3U"
