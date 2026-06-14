/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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

#include <OpenMPTVisBase.hpp>

#include <QImage>

#include <vector>

class OpenMPTVisSamples : public OpenMPTVisBase
{
public:
    OpenMPTVisSamples(openmpt::module *module);

    Frame render(openmpt::module *module) override;
    void reset() override;

private:
    int m_numSamples{0};
    int m_numChannels{0};
    int m_indexWidth{0};
    int m_channelWidth{0};
    int m_rowHeight{0};
    bool m_allowRender{true};
    bool m_isReset{true};
    QImage m_textBuffer;
    std::vector<int> m_lastInstrument;
};
