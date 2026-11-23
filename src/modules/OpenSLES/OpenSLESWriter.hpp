// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <OpenSLESHeaders.hpp>

#include <Writer.hpp>

class OpenSLESWriter final : public Writer
{
public:
    OpenSLESWriter(Module &);
private:
        ~OpenSLESWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    qint64 write(const QByteArray &) override;
    void pause() override;

    QString name() const override;

    bool open() override;

    /**/

    void close();

    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLObjectItf outputMixObject;
    SLObjectItf bqPlayerObject;
    SLPlayItf bqPlayerPlay;
    SLBufferQueueItf bqPlayerBufferQueue;

    QVector<QVector<qint16>> buffers;
    QVector<qint16> tmpBuffer;
    QSemaphore sem;
    int currBuffIdx;

    int sample_rate, channels;
    bool paused;
};

#define OpenSLESWriterName "OpenSL|ES"
