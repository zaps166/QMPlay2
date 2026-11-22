// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Reader.hpp>

#include <OpenThr.hpp>

struct AVIOContext;

class FFReader final : public Reader
{
public:
    FFReader();
private:
    bool readyRead() const override;
    bool canSeek() const override;

    bool seek(qint64) override;
    QByteArray read(qint64) override;
    void pause() override;
    bool atEnd() const override;
    void abort() override;

    qint64 size() const override;
    qint64 pos() const override;
    QString name() const override;

    bool open() override;

    /**/

    ~FFReader();

    AVIOContext *avioCtx;
    bool paused, canRead;
    std::shared_ptr<AbortContext> abortCtx;
};
