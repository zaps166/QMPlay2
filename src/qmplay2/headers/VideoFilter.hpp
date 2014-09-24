#ifndef VIDEOFILTER_HPP
#define VIDEOFILTER_HPP

#include <ModuleParams.hpp>

#include <QQueue>

class VideoFilter : public ModuleParams
{
public:
	class FrameBuffer
	{
	public:
		inline FrameBuffer( const QByteArray &data, double ts ) :
			data( data ), ts( ts )
		{}

		QByteArray data;
		double ts;
	};

	virtual ~VideoFilter()
	{
		clearBuffer();
	}

	void clearBuffer();
	bool removeLastFromInternalBuffer();

	virtual void filter( QQueue< FrameBuffer > &framesQueue ) = 0;
protected:
	int addFramesToInternalQueue( QQueue< FrameBuffer > &framesQueue );

	inline double halfDelay( const FrameBuffer &f1, const FrameBuffer &f2 ) const
	{
		return ( f1.ts - f2.ts ) / 2.0;
	}

	QQueue< FrameBuffer > internalQueue;
};

#endif
