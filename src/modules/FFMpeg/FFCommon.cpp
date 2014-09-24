#include <FFCommon.hpp>

#include <DeintFilter.hpp>

int FFCommon::getField( const VideoFrame *videoFrame, int deinterlace, int fullFrame, int topField, int bottomField )
{
	if ( deinterlace )
	{
		const quint8 deintFlags = deinterlace >> 1;
		if ( videoFrame->interlaced || !( deintFlags & DeintFilter::AutoDeinterlace ) )
		{
			bool topFieldFirst;
			if ( ( deintFlags & DeintFilter::DoubleFramerate ) || ( ( deintFlags & DeintFilter::AutoParity ) && videoFrame->interlaced ) )
				topFieldFirst = videoFrame->top_field_first;
			else
				topFieldFirst = deintFlags & DeintFilter::TopFieldFirst;
			return topFieldFirst ? topField : bottomField;
		}
	}
	return fullFrame;
}
