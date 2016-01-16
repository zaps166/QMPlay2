#include <PortAudioCommon.hpp>
#include <QMPlay2Core.hpp>

#include <portaudio.h>

#define getOutputDeviceName QString( hostApiInfo->name ) + ": " + QString::fromLocal8Bit( deviceInfo->name )

QStringList PortAudioCommon::getOutputDeviceNames()
{
	QStringList outputDeviceNames;
	int numDevices = Pa_GetDeviceCount();
	for ( int i = 0; i < numDevices; i++ )
	{
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );
		if ( deviceInfo )
		{
			const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
			if ( deviceInfo->maxOutputChannels > 0 )
				outputDeviceNames += getOutputDeviceName;
		}
	}
	return outputDeviceNames;
}
int PortAudioCommon::getDeviceIndexForOutput( const QString &name )
{
	int numDevices = Pa_GetDeviceCount();
	for ( int i = 0; i < numDevices; i++ )
	{
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );
		if ( deviceInfo )
		{
			const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
			if ( deviceInfo->maxOutputChannels > 0 && name == getOutputDeviceName )
				return i;
		}
	}
	return getDefaultOutputDevice();
}
int PortAudioCommon::getDefaultOutputDevice()
{
#ifdef Q_OS_LINUX
	if ( getOutputDeviceNames().contains( "ALSA: default" ) )
		return getDeviceIndexForOutput( "ALSA: default" );
#endif
	return Pa_GetDefaultOutputDevice();
}
int PortAudioCommon::deviceIndexToOutputIndex( int dev )
{
	const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( dev );
	if ( deviceInfo )
	{
		const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
		int idx = getOutputDeviceNames().indexOf( getOutputDeviceName );
		if ( idx > -1 )
			return idx;
	}
	return dev;
}
