#include <QMPlay2_OSD.hpp>

#include <QCryptographicHash>

void QMPlay2_OSD::genChecksum()
{
	QCryptographicHash hash( QCryptographicHash::Md4 );
	foreach ( Image img, images )
		hash.addData( img.data );
	checksum = hash.result();
}

void QMPlay2_OSD::clear( bool all )
{
	images.clear();
	_text.clear();
	if ( all )
		_pts = _duration = -1.0;
	_needsRescale = started = false;
	checksum.clear();
}

void QMPlay2_OSD::start()
{
	started = true;
	if ( _pts == -1.0 )
		timer.start();
}
double QMPlay2_OSD::left_duration()
{
	if ( !started || _pts != -1.0 )
		return 0.0;
	return _duration - timer.elapsed() / 1000.0;
}
