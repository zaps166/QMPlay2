#include <Playlist.hpp>

class PLS : public Playlist
{
	Entries _read();
	bool _write( const Entries & );

	~PLS() {}

	void prepareList( Entries *, int );
};

#define PLSName "PLS"
