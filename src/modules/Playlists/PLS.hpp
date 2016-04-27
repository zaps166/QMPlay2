#include <Playlist.hpp>

class PLS : public Playlist
{
	Entries read();
	bool write(const Entries &);

	~PLS();
};

#define PLSName "PLS"
