#ifndef DEMUXER_HPP
#define DEMUXER_HPP

#include <ModuleCommon.hpp>
#include <ChapterInfo.hpp>
#include <StreamInfo.hpp>
#include <Playlist.hpp>

#include <QString>

struct Packet;

class Demuxer : protected ModuleCommon, public BasicIO
{
public:
	class FetchTracks
	{
	public:
		inline FetchTracks(bool onlyTracks) :
			onlyTracks(onlyTracks),
			isOK(true)
		{}

		Playlist::Entries tracks;
		bool onlyTracks, isOK;
	};

	static bool create(const QString &url, IOController< Demuxer > &demuxer, FetchTracks *fetchTracks = NULL);

	virtual bool metadataChanged() const
	{
		return false;
	}

	inline QList< StreamInfo * > streamsInfo() const
	{
		return streams_info;
	}

	virtual QList< ChapterInfo > getChapters() const
	{
		return QList< ChapterInfo >();
	}

	virtual QString name() const = 0;
	virtual QString title() const = 0;
	virtual QList< QMPlay2Tag > tags() const
	{
		return QList< QMPlay2Tag >();
	}
	virtual bool getReplayGain(bool album, float &gain_db, float &peak) const
	{
		Q_UNUSED(album)
		Q_UNUSED(gain_db)
		Q_UNUSED(peak)
		return false;
	}
	virtual double length() const = 0;
	virtual int bitrate() const = 0;
	virtual QByteArray image(bool forceCopy = false) const
	{
		Q_UNUSED(forceCopy)
		return QByteArray();
	}

	virtual bool localStream() const
	{
		return true;
	}
	virtual bool dontUseBuffer() const
	{
		return false;
	}

	virtual bool seek(int) = 0;
	virtual bool read(Packet &, int &) = 0;

	virtual ~Demuxer() {}
private:
	virtual bool open(const QString &url) = 0;

	virtual Playlist::Entries fetchTracks(const QString &url, bool &ok)
	{
		Q_UNUSED(url)
		Q_UNUSED(ok)
		return Playlist::Entries();
	}
protected:
	StreamsInfo streams_info;
};

#endif
