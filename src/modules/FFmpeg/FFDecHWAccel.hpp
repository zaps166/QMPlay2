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

#pragma once

#include <FFDec.hpp>

class FFDecHWAccel : public FFDec
{
protected:
    FFDecHWAccel();
    ~FFDecHWAccel();

    bool hasHWAccel(const char *hwaccelName) const;

    bool hasHWDecContext() const override final;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurryUp) override;
    virtual void downloadVideoFrame(Frame &decoded);

    bool hasCriticalError() const override;

protected:
    bool m_hasCriticalError = false;
    bool m_hasHWDecContext = false;
};
