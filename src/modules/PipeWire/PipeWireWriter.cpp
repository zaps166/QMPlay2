/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <PipeWireWriter.hpp>

#include <QDebug>
#include <QtMath>

class LoopLocker
{
public:
    LoopLocker(pw_thread_loop *threadLoop)
        : m_threadLoop(threadLoop)
    {
        if (m_threadLoop)
            pw_thread_loop_lock(m_threadLoop);
    }
    ~LoopLocker()
    {
        unlock();
    }

    void unlock()
    {
        if (m_threadLoop)
        {
            pw_thread_loop_unlock(m_threadLoop);
            m_threadLoop = nullptr;
        }
    }

private:
    pw_thread_loop *m_threadLoop = nullptr;
};

/**/

PipeWireWriter::PipeWireWriter(Module &module)
{
    addParam("delay");
    addParam("chn");
    addParam("rate");
    addParam("drain");

    SetModule(module);
}
PipeWireWriter::~PipeWireWriter()
{
    if (!m_threadLoop)
        return;

    destroyStream(false);

    pw_thread_loop_stop(m_threadLoop);

    if (m_registry)
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(m_registry));

    if (m_core)
        pw_core_disconnect(m_core);

    if (m_context)
        pw_context_destroy(m_context);

    pw_thread_loop_destroy(m_threadLoop);
}

bool PipeWireWriter::set()
{
    return sets().getBool("WriterEnabled");
}

bool PipeWireWriter::readyWrite() const
{
    return m_stream && !m_err;
}

bool PipeWireWriter::processParams(bool *)
{
    bool doRecreateStream = !m_stream;

    uchar chn = getParam("chn").toUInt();
    if (m_chn != chn)
    {
        doRecreateStream  = true;
        m_chn = chn;
    }

    uint rate = getParam("rate").toUInt();
    if (m_rate != rate)
    {
        doRecreateStream = true;
        m_rate = rate;
    }

    if (doRecreateStream && !m_err)
    {
        recreateStream();
    }

    if (m_err)
    {
        QMPlay2Core.logError("PipeWire :: " + tr("Cannot open audio output stream"));
    }

    return readyWrite();
}
qint64 PipeWireWriter::write(const QByteArray &arr)
{
    if (!arr.size() || !readyWrite())
        return 0;

    if (m_paused.exchange(false))
    {
        LoopLocker locker(m_threadLoop);
        if (m_streamPaused)
            pw_stream_set_active(m_stream, true);
    }

    const int dataFrames = arr.size() / m_stride;
    if (m_readRem == 0 || m_readRem + m_readPos > dataFrames)
    {
        m_readRem = dataFrames;
        m_readPos = 0;
    }

    while (m_readRem > 0)
    {
        if (m_waitForProcessed)
        {
            LoopLocker locker(m_threadLoop);
            while (!m_err && !m_processed)
            {
                if (pw_thread_loop_timed_wait(m_threadLoop, 1) != 0)
                    return -1;
            }
            m_processed = false;

            m_waitForProcessed = false;
        }

        if (m_err)
            return 0;

        const int chunkSize = qMin<int>(m_nFrames - m_writeBufferPos, m_readRem);

        memcpy(m_buffer.get() + m_writeBufferPos * m_stride, arr.constData() + m_readPos * m_stride, chunkSize * m_stride);
        m_writeBufferPos += chunkSize;
        if (m_writeBufferPos >= m_nFrames)
        {
            m_writeBufferPos = 0;
            m_bufferHasData = true;
            m_waitForProcessed = true;
        }

        m_readRem -= chunkSize;
        m_readPos += chunkSize;
    }

    Q_ASSERT(m_readRem == 0);

    return arr.size();
}

QString PipeWireWriter::name() const
{
    return PipeWireWriterName;
}

bool PipeWireWriter::open()
{
    static const pw_core_events coreEvents = {
        .version = PW_VERSION_CORE_EVENTS,
        .done = [](void *object, uint32_t id, int seq) {
            reinterpret_cast<PipeWireWriter *>(object)->onCoreEventDone(id, seq);
        },
    };

    static const pw_registry_events registryEvents = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = [](void *object, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const spa_dict *props) {
            reinterpret_cast<PipeWireWriter *>(object)->onRegistryEventGlobal(id, permissions, type, version, props);
        },
    };

    m_threadLoop = pw_thread_loop_new("pipewire-loop", nullptr);
    if (!m_threadLoop)
    {
        m_err = true;
        return false;
    }

    m_context = pw_context_new(pw_thread_loop_get_loop(m_threadLoop), nullptr, 0);
    if (!m_context)
    {
        m_err = true;
        return false;
    }

    m_core = pw_context_connect(m_context, nullptr, 0);
    if (!m_core)
    {
        m_err = true;
        return false;
    }
    pw_core_add_listener(m_core, &m_coreListener, &coreEvents, this);

    m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
    if (!m_registry)
    {
        m_err = true;
        return false;
    }
    pw_registry_add_listener(m_registry, &m_registryListener, &registryEvents, this);

    updateCoreInitSeq();

    if (pw_thread_loop_start(m_threadLoop) != 0)
    {
        m_err = true;
        return false;
    }

    LoopLocker locker(m_threadLoop);
    while (!m_initDone)
    {
        if (pw_thread_loop_timed_wait(m_threadLoop, 2) != 0)
            break;
    }

    return m_initDone && m_hasSinks;
}

void PipeWireWriter::pause()
{
    m_paused = true;
}

void PipeWireWriter::onCoreEventDone(uint32_t id, int seq)
{
    if (id == PW_ID_CORE && seq == m_coreInitSeq)
    {
        spa_hook_remove(&m_registryListener);
        spa_hook_remove(&m_coreListener);

        m_initDone = true;
        pw_thread_loop_signal(m_threadLoop, false);
    }
}

void PipeWireWriter::onRegistryEventGlobal(uint32_t id, uint32_t permissions, const char *type, uint32_t version, const spa_dict *props)
{
    Q_UNUSED(id)
    Q_UNUSED(permissions)
    Q_UNUSED(version)

    if (qstrcmp(type, PW_TYPE_INTERFACE_Node) != 0)
        return;

    auto media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!media_class)
        return;

    if (qstrcmp(media_class, "Audio/Sink") != 0)
        return;

    m_hasSinks = true;

    updateCoreInitSeq();
}

void PipeWireWriter::onStateChanged(pw_stream_state old, pw_stream_state state, const char *error)
{
    Q_UNUSED(old)
    Q_UNUSED(error)

    if (m_ignoreStateChange)
        return;

    switch (state)
    {
        case PW_STREAM_STATE_UNCONNECTED:
            signalLoop(false, true);
            break;
        case PW_STREAM_STATE_PAUSED:
            m_streamPaused = true;
            signalLoop(false, false);
            break;
        case PW_STREAM_STATE_STREAMING:
            m_streamPaused = false;
            signalLoop(false, false);
            break;
        default:
            break;
    }
}
void PipeWireWriter::onProcess()
{
    auto b = pw_stream_dequeue_buffer(m_stream);
    if (!b)
        return;

    auto &d = b->buffer->datas[0];
    if (!d.data)
    {
        signalLoop(true, true);
        return;
    }

    if (m_bufferSize > d.maxsize)
    {
        signalLoop(true, true);
        return;
    }

    if (m_bufferHasData.exchange(false))
    {
        memcpy(d.data, m_buffer.get(), m_bufferSize);
        m_silence = false;
    }
    else
    {
        memset(d.data, 0, m_bufferSize);
        if (!m_silence.exchange(true))
            m_silenceElapsed.start();
    }

    signalLoop(true, false);

    d.chunk->offset = 0;
    d.chunk->size = m_bufferSize;
    d.chunk->stride = m_stride;

    pw_stream_queue_buffer(m_stream, b);

    if (m_silence && m_paused && m_silenceElapsed.isValid() && m_silenceElapsed.elapsed() >= 1000)
        pw_stream_set_active(m_stream, false);
}

void PipeWireWriter::updateCoreInitSeq()
{
    m_coreInitSeq = pw_core_sync(m_core, PW_ID_CORE, m_coreInitSeq);
}

void PipeWireWriter::recreateStream()
{
    static const pw_stream_events streamEvents = {
        .version = PW_VERSION_STREAM_EVENTS,
        .state_changed = [](void *data, pw_stream_state old, pw_stream_state state, const char *error) {
            reinterpret_cast<PipeWireWriter *>(data)->onStateChanged(old, state, error);
        },
        .process = [](void *data) {
            reinterpret_cast<PipeWireWriter *>(data)->onProcess();
        },
    };

    destroyStream(true);

    m_stride = sizeof(float) * m_chn;
    m_nFrames = qBound(64, 1 << qRound(log2((1024.0 / 48000.0) * m_rate)), 8192);
    m_bufferSize = m_nFrames * m_stride;
    m_writeBufferPos = 0;
    m_buffer = std::make_unique<uint8_t[]>(m_bufferSize);

    auto props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Playback",
        PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_MEDIA_ICON_NAME, "QMPlay2",
        nullptr
    );
    pw_properties_setf(props, PW_KEY_NODE_LATENCY, "%u/%u", m_nFrames, m_rate);

    LoopLocker locker(m_threadLoop);

    m_stream = pw_stream_new(m_core, "Playback", props);
    if (!m_stream)
    {
        m_err = true;
        return;
    }

    m_streamListener = {};
    pw_stream_add_listener(m_stream, &m_streamListener, &streamEvents, this);

    uint8_t buffer[1024];
    spa_pod_builder b = {
        .data = buffer,
        .size = sizeof(buffer)
    };

    spa_audio_info_raw info = {
        .format = SPA_AUDIO_FORMAT_F32,
        .flags = SPA_AUDIO_FLAG_NONE,
        .rate = m_rate,
        .channels = m_chn,
    };

    const spa_pod *params[2];
    params[0] = spa_format_audio_raw_build(
        &b,
        SPA_PARAM_EnumFormat,
        &info
    );
    params[1] = reinterpret_cast<spa_pod *>(spa_pod_builder_add_object(
        &b,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_size, SPA_POD_CHOICE_RANGE_Int(m_bufferSize, m_bufferSize, INT_MAX),
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(2, 1, 2)
    ));

    const int connectErr = pw_stream_connect(
        m_stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS),
        params,
        2
    );
    if (connectErr != 0)
    {
        m_err = true;
        return;
    }

    modParam("delay", 2.0 * m_nFrames / m_rate);
}
void PipeWireWriter::destroyStream(bool forceDrain)
{
    if (!m_stream)
        return;

    if (forceDrain || getParam("drain").toBool())
    {
        LoopLocker locker(m_threadLoop);
        while (!m_streamPaused && !m_silence && !m_err)
        {
            if (pw_thread_loop_timed_wait(m_threadLoop, 1) != 0)
                break;
        }
    }

    LoopLocker locker(m_threadLoop);
    m_ignoreStateChange = true;
    pw_stream_disconnect(m_stream);
    pw_stream_destroy(m_stream);
    m_ignoreStateChange = false;

    m_stream = nullptr;
}

void PipeWireWriter::signalLoop(bool onProcessDone, bool err)
{
    if (err)
        m_err = true;
    if (onProcessDone)
        m_processed = true;
    pw_thread_loop_signal(m_threadLoop, false);
}
