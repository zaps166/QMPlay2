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

#pragma once

#include <Writer.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>

#include <atomic>

#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>

class PipeWireWriter final : public Writer
{
    Q_DECLARE_TR_FUNCTIONS(PipeWireWriter)

public:
    PipeWireWriter(Module &module);
    ~PipeWireWriter();

private:
    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    qint64 write(const QByteArray &) override;

    QString name() const override;

    bool open() override;

    void pause() override;

private:
    void onCoreEventDone(uint32_t id, int seq);

    void onRegistryEventGlobal(uint32_t id, uint32_t permissions, const char *type, uint32_t version, const spa_dict *props);

    void onStateChanged(pw_stream_state old, pw_stream_state state, const char *error);
    void onProcess();

    void updateCoreInitSeq();

    void recreateStream();
    void destroyStream(bool forceDrain);

    void signalLoop(bool onProcessDone, bool err);

private:
    pw_thread_loop *m_threadLoop = nullptr;
    pw_context *m_context = nullptr;

    pw_core *m_core = nullptr;
    spa_hook m_coreListener = {};

    pw_registry *m_registry = nullptr;
    spa_hook m_registryListener = {};

    pw_stream *m_stream = nullptr;
    spa_hook m_streamListener = {};

    int m_coreInitSeq = 0;

    uint8_t m_chn = 0;
    uint32_t m_rate = 0;

    int m_readRem = 0;
    int m_readPos = 0;

    bool m_waitForProcessed = false;

    uint32_t m_stride = 0;
    uint32_t m_nFrames = 0;
    uint32_t m_bufferSize = 0;
    uint32_t m_writeBufferPos = 0;
    std::unique_ptr<uint8_t[]> m_buffer = nullptr;

    std::atomic_bool m_hasSinks {false};
    std::atomic_bool m_initDone {false};
    std::atomic_bool m_bufferHasData {false};
    std::atomic_bool m_processed {false};
    std::atomic_bool m_paused {false};
    std::atomic_bool m_silence {false};
    std::atomic_bool m_streamPaused {false};
    std::atomic_bool m_ignoreStateChange {false};
    std::atomic_bool m_err {false};

    QElapsedTimer m_silenceElapsed;
};

#define PipeWireWriterName "PipeWire"
