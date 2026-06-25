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

#include <OpenMPTVisSamples.hpp>

#include <QPainter>

#include <libopenmpt/libopenmpt.hpp>

extern "C" {
    #include <libavutil/frame.h>
    #include <libavutil/pixfmt.h>
}

static constexpr int kMaxRowHeight = 24;
static constexpr float kSilentThreshold = 1.0f / 255.0f;

static QFont computeFitFont(int nameWidth, int &outCharWidth, int &outAscent)
{
    QFont font = OpenMPTVisBase::visFont(OpenMPTVisBase::kDefaultFontPt);
    QFontMetrics fm(font);

    if (nameWidth > OpenMPTVisBase::kMaxVisSize)
    {
        const double scale = static_cast<double>(OpenMPTVisBase::kMaxVisSize) / nameWidth;
        const int pt = qMax(1, static_cast<int>(OpenMPTVisBase::kDefaultFontPt * scale));
        font = OpenMPTVisBase::visFont(pt);
        fm = QFontMetrics(font);
    }

    outCharWidth = fm.horizontalAdvance(QStringLiteral("X"));
    outAscent = fm.ascent();
    return font;
}

OpenMPTVisSamples::OpenMPTVisSamples(openmpt::module *module)
{
    std::vector<std::string> names;

    m_numSamples = module->get_num_instruments();
    if (m_numSamples <= 0)
    {
        m_numSamples = module->get_num_samples();
        if (m_numSamples <= 0)
        {
            return;
        }
        names = module->get_sample_names();
    }
    else
    {
        names = module->get_instrument_names();
    }

    m_numChannels = module->get_num_channels();
    if (m_numChannels <= 0)
        return;

    m_lastInstrument.assign(m_numChannels, -1);

    int maxNameLen = 0;
    for (const auto &name : names)
        maxNameLen = qMax(maxNameLen, static_cast<int>(name.length()));

    const int prefixLen = 3;
    const int maxLabelLen = prefixLen + maxNameLen;

    int charWidth = 0;
    int ascent = 0;
    QFont font = computeFitFont(maxLabelLen * 8, charWidth, ascent);

    m_indexWidth = prefixLen * charWidth;
    m_rowHeight = qMin(kMaxRowHeight, OpenMPTVisBase::kMaxVisSize / m_numSamples);
    m_channelWidth = m_rowHeight * 2;
    m_width = m_indexWidth + m_numChannels * m_channelWidth;
    if (m_width > OpenMPTVisBase::kMaxVisSize)
    {
        m_channelWidth = (OpenMPTVisBase::kMaxVisSize - m_indexWidth) / m_numChannels;
        m_width = m_indexWidth + m_numChannels * m_channelWidth;
    }
    m_height = m_numSamples * m_rowHeight;

    // Pre-render text labels into a buffer with transparent background
    m_textBuffer = QImage(m_width, m_height, QImage::Format_ARGB32);
    m_textBuffer.fill(Qt::transparent);

    QPainter p(&m_textBuffer);
    p.setFont(font);

    for (int i = 0; i < m_numSamples; ++i)
    {
        const int y = i * m_rowHeight;
        const QString indexStr = QString::number(i + 1, 16).toUpper().rightJustified(2, '0');
        const QString name = (i < static_cast<int>(names.size())) ? QString::fromStdString(names[i]) : QString();
        const QString label = indexStr + ' ' + name;

        p.setPen(Qt::white);
        p.drawText(0, y + ascent + 1, label);
    }

    p.end();
}

Frame OpenMPTVisSamples::render(openmpt::module *module)
{
    if (m_numSamples <= 0 || m_numChannels <= 0)
        return {};

    if (!m_allowRender)
    {
        m_allowRender = true;
        return {};
    }
    m_allowRender = false;

    const int curPattern = module->get_current_pattern();
    const int curRow = module->get_current_row();

    AVFrame *avFrame = av_frame_alloc();
    avFrame->width = m_width;
    avFrame->height = m_height;
    avFrame->format = AV_PIX_FMT_BGRA;

    if (av_frame_get_buffer(avFrame, 0) != 0)
    {
        av_frame_free(&avFrame);
        return {};
    }

    QImage img(avFrame->data[0], m_width, m_height, avFrame->linesize[0], QImage::Format_RGB32);

    // 1. Black background
    img.fill(Qt::black);

    // 2. Draw rectangles (under text)
    QPainter p(&img);
    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        int inst = module->get_pattern_row_channel_command(curPattern, curRow, ch, openmpt::module::command_instrument);
        if (inst == 0 && m_isReset && curRow > 0)
        {
            for (int row = curRow - 1; row >= 0; --row)
            {
                inst = module->get_pattern_row_channel_command(curPattern, row, ch, openmpt::module::command_instrument);
                if (inst > 0)
                {
                    break;
                }
            }
        }
        if (inst > 0)
        {
            m_lastInstrument[ch] = inst - 1;
        }

        const float vuL = module->get_current_channel_vu_left(ch);
        const float vuR = module->get_current_channel_vu_right(ch);
        if (vuL < kSilentThreshold && vuR < kSilentThreshold)
        {
            continue;
        }

        const int instIdx = m_lastInstrument[ch];
        if (instIdx < 0 || instIdx >= m_numSamples)
        {
            continue;
        }

        const int y = instIdx * m_rowHeight;
        const int chX = m_indexWidth + ch * m_channelWidth;

        p.fillRect(chX, y, m_channelWidth / 2, m_rowHeight, QColor(60, 130, 255, qRound(std::sqrt(vuL) * 255.0f)));
        p.fillRect(chX + m_channelWidth / 2, y, m_channelWidth / 2, m_rowHeight, QColor(60, 130, 255, qRound(std::sqrt(vuR) * 255.0f)));
    }

    // 3. Draw text buffer on top (text always readable)
    p.drawImage(0, 0, m_textBuffer);
    p.end();

    Frame frame(avFrame);
    av_frame_free(&avFrame);

    m_isReset = false;

    return frame;
}
void OpenMPTVisSamples::reset()
{
    m_allowRender = true;
    m_isReset = true;
    std::fill(m_lastInstrument.begin(), m_lastInstrument.end(), -1);
}
