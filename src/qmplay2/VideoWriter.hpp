// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Frame.hpp>
#include <Writer.hpp>
#include <QMPlay2OSD.hpp>

class HWDecContext;
class QMPlay2OSD;

class QMPLAY2SHAREDLIB_EXPORT VideoWriter : public Writer
{
public:
    VideoWriter();
    virtual ~VideoWriter();

    virtual AVPixelFormats supportedPixelFormats() const;

    qint64 write(const QByteArray &) override final;

    virtual void writeVideo(const Frame &videoFrame, QMPlay2OSDList &&osdList) = 0;

    virtual bool setHWDecContext(const std::shared_ptr<HWDecContext> &hwDecContext);
    virtual std::shared_ptr<HWDecContext> hwDecContext() const;

    virtual bool open() override = 0;
};
