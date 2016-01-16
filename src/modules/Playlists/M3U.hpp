#include <Playlist.hpp>

class M3U : public Playlist
{
	Entries _read();
	bool _write(const Entries &);

	~M3U() {}
};

#define M3UName "M3U"
