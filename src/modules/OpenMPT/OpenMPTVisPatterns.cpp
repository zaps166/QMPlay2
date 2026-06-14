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

#include <OpenMPTVisPatterns.hpp>

#include <QPainter>

#include <libopenmpt/libopenmpt.hpp>

extern "C" {
    #include <libavutil/frame.h>
    #include <libavutil/pixfmt.h>
}

static int computeMaxCellWidth(openmpt::module *module, int pattern, const QFontMetrics &fm)
{
    const int numChannels = module->get_num_channels();
    int maxW = 0;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const std::string text = module->format_pattern_row_channel(pattern, 0, ch);
        const int w = fm.horizontalAdvance(QString::fromStdString(text)) + fm.horizontalAdvance("X");
        if (w > maxW)
            maxW = w;
    }
    return qMax(maxW, 1);
}

static QFont computeFitFont(openmpt::module *module, int pattern, int visChannels, int &outMaxCellWidth)
{
    QFont font = OpenMPTVisBase::visFont(OpenMPTVisBase::kDefaultFontPt);

    const int maxCellWidth = computeMaxCellWidth(module, pattern, QFontMetrics(font));
    const int width = visChannels * maxCellWidth;

    if (width > OpenMPTVisBase::kMaxVisSize)
    {
        const double scale = static_cast<double>(OpenMPTVisBase::kMaxVisSize) / width;
        const int pt = qMax(1, static_cast<int>(OpenMPTVisBase::kDefaultFontPt * scale));
        font = OpenMPTVisBase::visFont(pt);
        outMaxCellWidth = computeMaxCellWidth(module, pattern, QFontMetrics(font));
    }
    else
    {
        outMaxCellWidth = maxCellWidth;
    }

    return font;
}

OpenMPTVisPatterns::OpenMPTVisPatterns(openmpt::module *module, int channels, int rows)
{
    const int pattern = module->get_current_pattern();
    const int numChannels = module->get_num_channels();

    m_channels = (channels <= 0) ? numChannels : qMin(numChannels, channels);
    m_userRows = qMax(1, rows);
    m_rows = m_userRows;

    int maxCellWidth = 0;
    m_font = computeFitFont(module, pattern, m_channels, maxCellWidth);
    const QFontMetrics fm(m_font);

    m_width = m_channels * maxCellWidth;
    m_rowHeight = fm.height() + 2;
    m_height = m_rows * m_rowHeight;
    m_maxCellWidth = maxCellWidth;
    m_charWidth = fm.horizontalAdvance("X");

    for (int i = 33; i <= 126; ++i)
    {
        QStaticText &st = m_charCache[i];
        st.setText(QString(QChar(i)));
        st.prepare(QTransform(), m_font);
    }
}

Frame OpenMPTVisPatterns::render(openmpt::module *module)
{
    const int curPattern = module->get_current_pattern();
    const int curRow = module->get_current_row();

    if (curPattern == m_lastPattern && curRow == m_lastRow)
        return {};

    m_lastPattern = curPattern;
    m_lastRow = curRow;

    if (curPattern < 0 || curRow < 0)
        return {};

    const int numRows = module->get_pattern_num_rows(curPattern);
    if (numRows <= 0)
        return {};

    const int numChannels = module->get_num_channels();
    if (numChannels <= 0)
        return {};

    const int newRows = qMin(numRows, m_userRows);
    if (newRows != m_rows)
    {
        m_rows = newRows;
        m_height = m_rows * m_rowHeight;
    }

    const int firstRow = qMax(0, curRow - m_rows / 2);
    const int lastRow = qMin(numRows - 1, firstRow + m_rows - 1);

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
    img.fill(Qt::black);

    QPainter p(&img);
    p.setFont(m_font);

    for (int row = firstRow; row <= lastRow; ++row)
    {
        const int y = (row - firstRow) * m_rowHeight;

        if (row == curRow)
            p.fillRect(0, y, m_width, m_rowHeight, QColor(30, 30, 60));

        for (int ch = 0; ch < m_channels; ++ch)
        {
            const int x = ch * m_maxCellWidth;

            const std::string text = module->format_pattern_row_channel(curPattern, row, ch);
            const std::string hl = module->highlight_pattern_row_channel(curPattern, row, ch);

            const QString qText = QString::fromStdString(text);
            const QString qHl = QString::fromStdString(hl);

            for (int i = 0; i < qText.size(); ++i)
            {
                const QChar c = qText.at(i);
                if (c == ' ')
                    continue;

                char hlChar = (i < qHl.size()) ? qHl.at(i).toLatin1() : ' ';
                QColor color = [hlChar]() {
                    switch (hlChar)
                    {
                        case 'n': return QColor(0, 255, 0);
                        case 'm': return QColor(0, 255, 128);
                        case 'i': return QColor(0, 255, 255);
                        case 'u': return QColor(255, 255, 0);
                        case 'v': return QColor(200, 200, 0);
                        case 'e': return QColor(255, 80, 80);
                        case 'f': return QColor(255, 120, 255);
                        case '.': return QColor(100, 100, 100);
                        default:  return QColor(180, 180, 180);
                    }
                }();

                if (row == curRow)
                    color = color.lighter(130);

                p.setPen(color);

                const QPoint textPos(x + i * m_charWidth, y + 1);
                if (Q_LIKELY(c.unicode() >= 33 && c.unicode() <= 126))
                    p.drawStaticText(textPos, m_charCache[c.unicode()]);
                else
                    p.drawText(textPos, c);
            }
        }
    }

    p.end();

    Frame frame(avFrame);
    av_frame_free(&avFrame);

    return frame;
}
void OpenMPTVisPatterns::reset()
{
    m_lastPattern = -1;
    m_lastRow = -1;
}
