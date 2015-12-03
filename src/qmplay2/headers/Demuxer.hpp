#ifndef DEMUXER_HPP
#define DEMUXER_HPP

#include <ModuleCommon.hpp>
#include <StreamInfo.hpp>
#include <Playlist.hpp>

#include <QString>

struct Packet;

class Demuxer : protected ModuleCommon, public BasicIO
{
public:
	class ChapterInfo
	{
	public:
		inline ChapterInfo( double start, double end ) :
			start( start ), end( end )
		{}

		QString title;
		double start, end;
	};

	static bool create( const QString &url, IOController< Demuxer > &demuxer, Playlist::Entries *tracks = NULL, bool onlyTracks = false );

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
	virtual bool getReplayGain( bool album, float &gain_db, float &peak ) const
	{
		Q_UNUSED( album )
		Q_UNUSED( gain_db )
		Q_UNUSED( peak )
		return false;
	}
	virtual double length() const = 0;
	virtual int bitrate() const = 0;
	virtual QByteArray image( bool forceCopy = false ) const
	{
		Q_UNUSED( forceCopy )
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

	virtual bool seek( int, bool backward = false ) = 0;
	virtual bool read( Packet &, int & ) = 0;

	virtual ~Demuxer() {}
private:
	virtual bool open( const QString &url ) = 0;

	virtual Playlist::Entries fetchTracks( const QString &url )
	{
		Q_UNUSED( url )
		return Playlist::Entries();
	}
protected:
	class StreamsInfo : public QList< StreamInfo  * >
	{
	public:
		inline ~StreamsInfo()
		{
			for ( int i = 0 ; i < count() ; ++i )
				delete at( i );
		}
	} streams_info;
};

#endif
