/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2023  Błażej Szczygieł

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

/*
    This is the C++/Qt port of the Yadif deinterlacing filter from FFmpeg (libavfilter/vf_yadif.c)
    Copyright (C) 2006-2011 Michael Niedermayer <michaelni@gmx.at>
*/

#include <YadifDeint.hpp>

#include <QMPlay2Core.hpp>

#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <vector>

using namespace std;

/* Yadif algo */

template<int j>
static inline void check(const quint8 *const curr,
                         const int prefs, const int mrefs,
                         int &spatialScore, int &spatialPred)
{
    const int score = abs(curr[mrefs-1+j] - curr[prefs-1-j]) + abs(curr[mrefs+j] - curr[prefs-j]) + abs(curr[mrefs+1+j] - curr[prefs+1-j]);
    if (score < spatialScore)
    {
        // This must be in a separate condition w/o assigning condition to a variable,
        // otherwise the compiler will not optimize it.
        spatialScore = score;
    }
    if (score < spatialScore)
    {
        spatialPred = (curr[mrefs+j] + curr[prefs-j]) >> 1;
        switch (j)
        {
            case -1:
                check<-2>(curr, prefs, mrefs, spatialScore, spatialPred);
                break;
            case +1:
                check<+2>(curr, prefs, mrefs, spatialScore, spatialPred);
                break;
            default:
                break;
        }
    }
}
template<bool isNotEdge>
static inline void filterLine(quint8 *dest, const void *const destEnd,
                              const quint8 *__restrict__ prev, const quint8 *__restrict__ curr, const quint8 *__restrict__ next,
                              const qptrdiff prefs, const qptrdiff mrefs,
                              const int spatialCheck, const bool filterParity)
{
    const quint8 *prev2 = filterParity ? prev : curr;
    const quint8 *next2 = filterParity ? curr : next;
    while (dest != destEnd)
    {
        const int c = curr[mrefs];
        const int d = (prev2[0] + next2[0]) >> 1;
        const int e = curr[prefs];
        const int temporalDiff0 = abs(prev2[0] - next2[0]);
        const int temporalDiff1 = (abs(prev[mrefs] - c) + abs(prev[prefs] - e)) >> 1;
        const int temporalDiff2 = (abs(next[mrefs] - c) + abs(next[prefs] - e)) >> 1;

        int diff = max({temporalDiff0 >> 1, temporalDiff1, temporalDiff2});
        int spatialPred = (c + e) >> 1;

        /* Reads 3 pixels to the left/right */
        if (isNotEdge)
        {
            int spatialScore = abs(curr[mrefs-1] - curr[prefs-1]) + abs(c - e) + abs(curr[mrefs+1] - curr[prefs+1]) - 1;
            check<-1>(curr, prefs, mrefs, spatialScore, spatialPred);
            check<+1>(curr, prefs, mrefs, spatialScore, spatialPred);
        }

        /* Spatial interlacing check */
        if (spatialCheck)
        {
            const int b = (prev2[2 * mrefs] + next2[2 * mrefs]) >> 1;
            const int f = (prev2[2 * prefs] + next2[2 * prefs]) >> 1;
            const int maxVal = max({d - e, d - c, min(b - c, f - e)});
            const int minVal = min({d - e, d - c, max(b - c, f - e)});
            diff = max({diff, minVal, -maxVal});
        }

        if (spatialPred > d + diff)
            spatialPred = d + diff;
        if (spatialPred < d - diff)
            spatialPred = d - diff;

        *(dest++) = spatialPred;
        ++prev;
        ++curr;
        ++next;
        ++prev2;
        ++next2;
    }
}

static void filterSlice(const int plane, const int parity, const int tff, const bool spatialCheck,
                        Frame &destFrame, const Frame &prevFrame, const Frame &currFrame, const Frame &nextFrame,
                        const int jobId, const int jobsCount)
{
    const int w = currFrame.width(plane);
    const int h = currFrame.height(plane);

    const int sliceStart   = (h *  jobId   ) / jobsCount;
    const int sliceEnd     = (h * (jobId+1)) / jobsCount;
    const int refs         = currFrame.linesize(plane);
    const int destLinesize = destFrame.linesize(plane);
    const int filterParity = parity ^ tff;

    const quint8 *const prevData = prevFrame.constData(plane);
    const quint8 *const currData = currFrame.constData(plane);
    const quint8 *const nextData = nextFrame.constData(plane);
    quint8 *const destData = destFrame.data(plane);

    constexpr int toSub = 3;

    for (int y = sliceStart; y < sliceEnd; ++y)
    {
        const quint8 *curr  = &currData[y * refs];
        quint8 *dest = &destData[y * destLinesize];
        if ((y ^ parity) & 1)
        {
            const quint8 *prev  = &prevData[y * refs];
            const quint8 *next  = &nextData[y * refs];

            const int prefs = (y + 1) < h ? refs : -refs;
            const int mrefs = y ? -refs : refs;

            const int doSpatialCheck = (y == 1 || y + 2 == h) ? false : spatialCheck;

            filterLine<false>
            (
                dest,
                dest + 3,
                prev,
                curr,
                next,
                prefs,
                mrefs,
                doSpatialCheck,
                filterParity
            );
            filterLine<true>
            (
                dest  + 3,
                dest  + w - toSub,
                prev  + 3,
                curr  + 3,
                next  + 3,
                prefs,
                mrefs,
                doSpatialCheck,
                filterParity
            );
            filterLine<true>
            (
                dest  + w - toSub,
                dest  + w - 3,
                prev  + w - toSub,
                curr  + w - toSub,
                next  + w - toSub,
                prefs,
                mrefs,
                doSpatialCheck,
                filterParity
            );
            filterLine<false>
            (
                dest  + w - 3,
                dest  + w,
                prev  + w - 3,
                curr  + w - 3,
                next  + w - 3,
                prefs,
                mrefs,
                doSpatialCheck,
                filterParity
            );
        }
        else
        {
            memcpy(dest, curr, w);
        }
    }
}

/* Yadif deint filter */

YadifDeint::YadifDeint(bool doubler, bool spatialCheck)
    : VideoFilter(true)
    , m_doubler(doubler)
    , m_spatialCheck(spatialCheck)
{
    m_threadsPool.setMaxThreadCount(min(QThread::idealThreadCount(), 18));
    addParam("DeinterlaceFlags");
    addParam("W");
    addParam("H");
}

bool YadifDeint::filter(QQueue<Frame> &framesQueue)
{
    addFramesToDeinterlace(framesQueue);
    if (m_internalQueue.count() >= 3)
    {
        const Frame &prevFrame = m_internalQueue.at(0);
        const Frame &currFrame = m_internalQueue.at(1);
        const Frame &nextFrame = m_internalQueue.at(2);

        Frame destFrame = getNewFrame(currFrame);
        destFrame.setNoInterlaced();

        auto doFilter = [&](const int jobId, const int jobsCount) {
            const bool tff = isTopFieldFirst(currFrame);
            for (int p = 0; p < 3; ++p)
            {
                filterSlice
                (
                    p,
                    m_secondFrame == tff, tff,
                    m_spatialCheck,
                    destFrame, prevFrame, currFrame, nextFrame,
                    jobId, jobsCount
                );
            }
        };

        const int threadsCount = min(m_threadsPool.maxThreadCount(), destFrame.height(1));

        vector<QFuture<void>> threads;
        threads.reserve(threadsCount);

        for (int i = 1; i < threadsCount; ++i)
            threads.push_back(QtConcurrent::run(&m_threadsPool, doFilter, i, threadsCount));
        doFilter(0, threadsCount);

        for (auto &&thread : threads)
            thread.waitForFinished();

        if (m_doubler)
            deinterlaceDoublerCommon(destFrame);
        else
            m_internalQueue.removeFirst();

        framesQueue.enqueue(destFrame);
    }
    return m_internalQueue.count() >= 3;
}

bool YadifDeint::processParams(bool *)
{
    processParamsDeint();
    if (getParam("W").toInt() < 3 || getParam("H").toInt() < 3 || (m_doubler == !(m_deintFlags & DoubleFramerate)))
        return false;
    return true;
}
