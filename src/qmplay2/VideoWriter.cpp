// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <VideoWriter.hpp>

VideoWriter::VideoWriter()
{}
VideoWriter::~VideoWriter()
{}

AVPixelFormats VideoWriter::supportedPixelFormats() const
{
    return {AV_PIX_FMT_YUV420P};
}

qint64 VideoWriter::write(const QByteArray &)
{
    return -1;
}

bool VideoWriter::setHWDecContext(const std::shared_ptr<HWDecContext> &hwDecContext)
{
    Q_UNUSED(hwDecContext)
    return false;
}
std::shared_ptr<HWDecContext> VideoWriter::hwDecContext() const
{
    return nullptr;
}
