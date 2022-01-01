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

#include <Classic.hpp>

#include <Functions.hpp>
#include <LibASS.hpp>

#include <QStringList>
#include <QRegExp>

#include <algorithm>
#include <cstdlib>
#include <cstdio>

/**
 * TMP      - hh:mm:ss:text    - "|" breaks line
 * MPL2     - [begin][end]text - deciseconds, divide by 10.0 for seconds, "|" breaks line
 * MicroDVD - {begin}{end}text - frames, use FPS for seconds calculation, "|" breaks line; TODO: {DEFAULT}
**/

class SubWithoutEnd
{
public:
    inline SubWithoutEnd(unsigned start, double duration, const QString &sub) :
        start(start), duration(duration), sub(sub)
    {}

    inline void setDuration(double duration)
    {
        if (duration < this->duration)
            this->duration = duration;
    }

    inline void operator +=(const SubWithoutEnd &other)
    {
        sub += '\n' + other.sub;
    }
    inline operator unsigned() const
    {
        return start;
    }

    unsigned start;
    double duration;
    QString sub;
};

static inline void initOnce(bool &ok, LibASS *ass)
{
    if (!ok)
    {
        ass->initASS();
        ok = true;
    }
}

static inline QString convertLine(const QRegExp &rx, const QString &line)
{
    return line.section(rx, 1, 1, QString::SectionIncludeTrailingSep).replace('|', '\n');
}

static void replaceText(QString &sub, int &pos, const int matchedLength, const bool singleLine, const QString &replaced, const QString &lf)
{
    sub.replace(pos, matchedLength, replaced);
    pos += replaced.length();
    if (singleLine)
    {
        const int idx = sub.indexOf('\n', pos);
        if (idx > -1)
            sub.insert(idx, lf);
    }
}

/**/

Classic::Classic(bool Use_mDVD_FPS, double Sub_max_s) :
    Use_mDVD_FPS(Use_mDVD_FPS), Sub_max_s(Sub_max_s)
{}

bool Classic::toASS(const QByteArray &txt, LibASS *ass, double fps)
{
    if (!ass)
        return false;

    bool ok = false, use_mDVD_FPS = Use_mDVD_FPS;

    const QRegExp TMPRegExp("\\d{1,2}:\\d{1,2}:\\d{1,2}\\D\\s?");
    const QRegExp MPL2RegExp("\\[\\d+\\]\\[\\d*\\]\\s?");
    const QRegExp MicroDVDRegExp("\\{\\d+\\}\\{\\d*\\}\\s?");
    QRegExp MicroDVDStylesRegExp("\\{(\\w):(.*)\\}");
    MicroDVDStylesRegExp.setMinimal(true);

    QList<SubWithoutEnd> subsWithoutEnd;

    for (const QString &line : QString(txt).remove('\r').split('\n', QString::SkipEmptyParts))
    {
        double start = 0.0, duration = 0.0;
        QString sub;
        int idx;

        if ((idx = line.indexOf(TMPRegExp)) > -1)
        {
            int h = -1, m = -1, s = -1;
            sscanf(line.toLatin1().constData() + idx, "%d:%d:%d", &h, &m, &s);
            if (h > -1 && m > -1 && s > -1)
            {
                start = h*3600 + m*60 + s;
                sub = convertLine(TMPRegExp, line);
            }
        }
        else if ((idx = line.indexOf(MPL2RegExp)) > -1)
        {
            int s = -1, e = -1;
            sscanf(line.toLatin1().constData() + idx, "[%d][%d]", &s, &e);
            if (s > -1)
            {
                for (const QString &l : convertLine(MPL2RegExp, line).split('\n'))
                {
                    if (!sub.isEmpty())
                        sub.append('\n');
                    if (!l.isEmpty())
                    {
                        switch (l.at(0).toLatin1()) {
                            case '/':
                                sub.append("{\\i1}" + l.mid(1) + "{\\i0}");
                                break;
                            case '\\':
                                sub.append("{\\b1}" + l.mid(1) + "{\\b0}");
                                break;
                            case '_':
                                sub.append("{\\u1}" + l.mid(1) + "{\\u0}");
                                break;
                            default:
                                sub.append(l);
                                break;
                        }
                    }
                }
                start = s / 10.0;
                duration = e / 10.0 - start;
            }
        }
        else if ((idx = line.indexOf(MicroDVDRegExp)) > -1)
        {
            int s = -1, e = -1;
            sscanf(line.toLatin1().constData() + idx, "{%d}{%d}", &s, &e);
            if (s > -1)
            {
                sub = convertLine(MicroDVDRegExp, line);

                if (use_mDVD_FPS && (s == 0 || s == 1))
                {
                    use_mDVD_FPS = false;
                    const double newFPS = sub.midRef(0, 6).toDouble();
                    if (newFPS > 0.0 && newFPS < 100.0)
                    {
                        fps = newFPS;
                        continue;
                    }
                }

                int pos = 0;
                while ((pos = MicroDVDStylesRegExp.indexIn(sub, pos)) != -1)
                {
                    const int matchedLength = MicroDVDStylesRegExp.matchedLength();
                    const QString styleText = MicroDVDStylesRegExp.cap(2);
                    const QChar s = MicroDVDStylesRegExp.cap(1).at(0);
                    const bool singleLine = s.isLower();
                    switch (s.toLower().toLatin1())
                    {
                        case 'c':
                            if (styleText.startsWith('$') && styleText.length() == 7)
                            {
                                replaceText(sub, pos, matchedLength, singleLine, "{\\1c&" + styleText.mid(1) + "&}", "{\\1c}");
                                continue;
                            }
                            break;
                        case 'f':
                            replaceText(sub, pos, matchedLength, singleLine, "{\\fn" + styleText + "}", "{\\fn}");
                            continue;
                        case 's':
                            replaceText(sub, pos, matchedLength, singleLine, "{\\fs" + styleText + "}", "{\\fs}");
                            continue;
                        case 'p':
                            if (!singleLine)
                            {
                                replaceText(sub, pos, matchedLength, false, "{\\pos(" + styleText + ")}", QString());
                                continue;
                            }
                            break;
                        case 'y':
                            replaceText(sub, pos, matchedLength, singleLine, "{\\" + styleText + "1}", "{\\" + styleText + "0}");
                            continue;
                    }
                    pos += MicroDVDStylesRegExp.matchedLength();
                }

                start = s / fps;
                duration = e / fps - start;
            }
        }

        if (start >= 0.0 && !sub.isEmpty())
        {
            if (duration > 0.0)
            {
                initOnce(ok, ass);
                ass->addASSEvent(Functions::convertToASS(sub), start, duration);
            }
            else
                subsWithoutEnd.append(SubWithoutEnd(start, Sub_max_s, sub));
        }
    }

    if (!subsWithoutEnd.isEmpty())
    {
        std::sort(subsWithoutEnd.begin(), subsWithoutEnd.end());

        for (int i = 0; i < subsWithoutEnd.size()-1; ++i)
        {
            const unsigned diff = subsWithoutEnd.at(i+1) - subsWithoutEnd.at(i);
            if (!diff)
            {
                subsWithoutEnd[i+1] += subsWithoutEnd.at(i);
                subsWithoutEnd.removeAt(i);
                --i;
            }
            else
                subsWithoutEnd[i].setDuration(diff);
        }

        initOnce(ok, ass);
        for (const SubWithoutEnd &sub : qAsConst(subsWithoutEnd))
            ass->addASSEvent(Functions::convertToASS(sub.sub), sub.start, sub.duration);
    }

    return ok;
}
