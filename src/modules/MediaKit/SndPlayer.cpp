#include <SndPlayer.hpp>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SoundPlayer.h>


static void proc(void *cookie, void *buffer, size_t len, const media_raw_audio_format &format)
{
    RingBuffer *ring = (RingBuffer*)cookie;
    unsigned char* ptr = (unsigned char*)buffer;

    int readed = ring->Read(ptr,len);

    if(readed <len)
        memset(ptr+readed, 0, len - readed);
}

SndPlayer::SndPlayer()
{
    channels = sample_rate = delay = 0;
    player = nullptr;
    _isOK = true;
}

bool SndPlayer::start()
{
    size_t gSoundBufferSize = 8192 * sizeof(float);

    media_raw_audio_format form = {
        sample_rate,
        channels,
        media_raw_audio_format::B_AUDIO_FLOAT,
        B_MEDIA_LITTLE_ENDIAN,
        gSoundBufferSize
    };

    ring = new RingBuffer(gSoundBufferSize * 3);
    if(ring->InitCheck() != B_OK) {
        delete ring; ring = 0;
        return false;
    }

    player = new BSoundPlayer(&form, "QMPlay2_BSoundPlayer", proc, nullptr, (void*)ring);

    if(player->InitCheck() != B_OK) {
        delete player;
        player = nullptr;
        return false;
    }

    player->Start();
    player->SetHasData(true);

    _isOK = true;

    return player;
}
void SndPlayer::stop()
{
    if ( player )
    {
        if(player) {
            player->Stop();
            delete player;
            delete ring;
        }

        player = nullptr;
        ring = nullptr;
    }
}

double SndPlayer::getLatency()
{
    double lat = player->Latency() / (ring->GetSize()*4.0);

    return lat;
}

bool SndPlayer::write( const QByteArray &arr )
{
    int s = arr.size();
    while ( s > 0 && s % 4 )
        s--;
    if ( s <= 0 )
        return false;

    int len=s;

    unsigned char *src_ptr = (unsigned char *)arr.data();

    for(;;) {
        int len2 = ring->Write(src_ptr,len);
        if(len2 == len)break;
        len -= len2;
        src_ptr += len2;
        snooze(100);
    }

    return true;
}
