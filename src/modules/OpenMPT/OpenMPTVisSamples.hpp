// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
