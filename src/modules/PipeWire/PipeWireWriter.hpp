// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <PipeWireHeaders.hpp>

#include <Writer.hpp>

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
