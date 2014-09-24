#ifndef PORTAUDIOCOMMON_HPP
#define PORTAUDIOCOMMON_HPP

#include <QStringList>

namespace PortAudioCommon
{
	QStringList getOutputDeviceNames();
	int getDeviceIndexForOutput( const QString & );
	int getDefaultOutputDevice();
	int deviceIndexToOutputIndex( int );
}

#endif
