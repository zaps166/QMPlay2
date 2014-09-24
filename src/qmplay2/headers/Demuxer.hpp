#ifndef DEMUXER_HPP
#define DEMUXER_HPP

#include <ModuleCommon.hpp>
#include <StreamInfo.hpp>
#include <TimeStamp.hpp>

#include <QString>

class Demuxer : public ModuleCommon
{
public:
	class ChapterInfo
	{
	public:
		inline ChapterInfo() :
			start( 0.0 ), end( 0.0 )
		{}

		QString title;
		double start, end;
	};

	static bool create( const QString &, Demuxer *&, const bool &br = false, QMutex *demuxerMutex = NULL );

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
	virtual bool getReplayGain( bool album, float &gain_db, float &peak )
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
	virtual bool read( QByteArray &, int &, TimeStamp &, double & ) = 0;
	virtual void pause() {}
	virtual void abort() {} //must be thread-safe!

	virtual ~Demuxer() {}
private:
	virtual bool open( const QString & ) = 0;
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
