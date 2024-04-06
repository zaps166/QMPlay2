#include <MediaKitWriter.hpp>
#include <QMPlay2Core.hpp>

MediaKitWriter::MediaKitWriter( Module &module ) :
    err( false )
{
    addParam( "delay" );
    addParam( "chn" );
    addParam( "rate" );

    SetModule( module );
}

bool MediaKitWriter::set()
{
    if ( player.delay != sets().getDouble( "Delay" ) )
    {
        player.delay = sets().getDouble( "Delay" );
        return false;
    }
    return sets().getBool( "WriterEnabled" );
}

bool MediaKitWriter::readyWrite() const
{
    return !err && player.isOpen();
}

bool MediaKitWriter::processParams( bool * )
{
    bool resetAudio = false;

    uchar chn = getParam( "chn" ).toUInt();
    if ( player.channels != chn )
    {
        resetAudio = true;
        player.channels = chn;
    }
    uint rate = getParam( "rate" ).toUInt();
    if ( player.sample_rate != rate )
    {
        resetAudio = true;
        player.sample_rate = rate;
    }

    if ( resetAudio || err )
    {
        player.stop();
        err = !player.start();
        if ( !err )
            modParam( "delay", player.delay );
        else
            QMPlay2Core.logError("MediaKitWriter :: Cannot open audio output stream");
    }

    return readyWrite();
}
qint64 MediaKitWriter::write( const QByteArray &arr )
{
    if ( !arr.size() || !readyWrite() )
        return 0;

    err = !player.write( arr );
    if ( err )
    {
        QMPlay2Core.logError("MediaKitWriter :: Playback error");
        return 0;
    }

    return arr.size();
}

qint64 MediaKitWriter::size() const
{
    return -1;
}
QString MediaKitWriter::name() const
{
    return MediaKitWriterName;
}

bool MediaKitWriter::open()
{
    return player.isOK();
}
