/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Pulse.hpp>

static void context_state_cb(pa_context *c, void *userdata)
{
    pa_threaded_mainloop *mainloop = (pa_threaded_mainloop *)userdata;
    switch (pa_context_get_state(c))
    {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            pa_threaded_mainloop_signal(mainloop, 0);
            break;
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
    }
}

/**/

Pulse::Pulse() :
    delay(0.0),
    channels(0),
    sample_rate(0),
    _isOK(false), writing(false),
    pulse(nullptr)
{
    ss.format = PA_SAMPLE_FLOAT32NE;

    /* test */
    pa_threaded_mainloop *mainloop = pa_threaded_mainloop_new();
    if (!mainloop)
        return;
    pa_context *context = pa_context_new(pa_threaded_mainloop_get_api(mainloop), "QMPlay2");
    if (context)
    {
        pa_context_set_state_callback(context, context_state_cb, mainloop);
        if (pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) >= 0)
        {
            pa_threaded_mainloop_lock(mainloop);
            if (pa_threaded_mainloop_start(mainloop) >= 0)
            {
                for (;;)
                {
                    pa_context_state_t state = pa_context_get_state(context);
                    _isOK = state == PA_CONTEXT_READY;
                    if (_isOK || !PA_CONTEXT_IS_GOOD(state))
                        break;
                    pa_threaded_mainloop_wait(mainloop);
                }
            }
            pa_threaded_mainloop_unlock(mainloop);
            pa_threaded_mainloop_stop(mainloop);
            pa_context_disconnect(context);
        }
        pa_context_unref(context);
    }
    pa_threaded_mainloop_free(mainloop);
    /**/
}

bool Pulse::start()
{
    pa_buffer_attr attr;
    attr.maxlength = (uint32_t) -1;
    attr.tlength = delay * (sample_rate * 4 * channels);
    attr.prebuf = (uint32_t) -1;
    attr.minreq = (uint32_t) -1;
    attr.fragsize = attr.tlength;

    ss.channels = channels;
    ss.rate = sample_rate;

    pa_channel_map *chn_map = nullptr;
    if (channels > 2 && channels <= 8)
    {
        chn_map = new pa_channel_map;
        chn_map->channels = channels;
        chn_map->map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
        chn_map->map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
        chn_map->map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
        chn_map->map[3] = PA_CHANNEL_POSITION_LFE;
        chn_map->map[4] = PA_CHANNEL_POSITION_REAR_LEFT;
        chn_map->map[5] = PA_CHANNEL_POSITION_REAR_RIGHT;
        chn_map->map[6] = PA_CHANNEL_POSITION_SIDE_LEFT;
        chn_map->map[7] = PA_CHANNEL_POSITION_SIDE_RIGHT;
    }

    pulse = pa_simple_new(nullptr, "QMPlay2", PA_STREAM_PLAYBACK, nullptr, "Output", &ss, chn_map, &attr, nullptr);

    delete chn_map;

    return pulse;
}
void Pulse::stop(bool drain)
{
    if (pulse)
    {
        if (!writing) //If "pa_simple_write()" is freezed and the audio thread is killed - don't free the context to avoid the deadlock
        {
            if (drain)
                pa_simple_drain(pulse, nullptr);
            pa_simple_free(pulse);
        }
        pulse = nullptr;
    }
}

bool Pulse::write(const QByteArray &arr, bool &showError)
{
    int error = 0;
    writing = true;
    const bool ret = pa_simple_write(pulse, arr.constData(), arr.size(), &error) >= 0;
    writing = false;
    if (error == PA_ERR_KILLED)
        showError = false;
    return (ret || error == PA_ERR_INVALID);
}
