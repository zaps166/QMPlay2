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

/*
    This is the C++/Qt port of the Yadif deinterlacing filter from FFmpeg (libavfilter/vf_yadif.c)
    Copyright (C) 2006-2011 Michael Niedermayer <michaelni@gmx.at>

    The Yadif x86 SIMD implementation is taken from:
    - FFmpeg project (libavfilter/x86/vf_yadif.asm)
        Copyright (C) 2006-2011 Michael Niedermayer <michaelni@gmx.at>
    - MPlayer project (libmpcodecs/vf_yadif.c)
        Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>
*/

#include <YadifDeint.hpp>

#include <QMPlay2Core.hpp>
#include <CPU.hpp>

#include <QtConcurrent/QtConcurrentRun>

extern "C" {
    #include <libavutil/cpu.h>
}

#include <algorithm>
#include <vector>

using namespace std;

static void (*filterLinePtr)(quint8 *, const void *const,
                             const quint8 *, const quint8 *, const quint8 *,
                             const qptrdiff, const qptrdiff,
                             const int, const bool);
static int alignment;

/* Yadif algo */

template<int j>
static inline void check(const quint8 *const curr,
                         const int prefs, const int mrefs,
                         int &spatialScore, int &spatialPred)
{
    const int score = abs(curr[mrefs-1+j] - curr[prefs-1-j]) + abs(curr[mrefs+j] - curr[prefs-j]) + abs(curr[mrefs+1+j] - curr[prefs+1-j]);
    if (score < spatialScore)
    {
        spatialScore = score;
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
                              const quint8 *prev, const quint8 *curr, const quint8 *next,
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
        else if (spatialPred < d - diff)
            spatialPred = d - diff;

        *(dest++) = spatialPred;
        ++prev;
        ++curr;
        ++next;
        ++prev2;
        ++next2;
    }
}

#ifdef QMPLAY2_CPU_X86

#define LOAD(mem, dst) \
    _movh "    " mem  ", " dst"\n\t"\
    "punpcklbw " mm(7)", " dst"\n\t"

#define PABS(tmp, dst) \
    "pxor    " tmp ", " tmp"\n\t"\
    "psubw   " dst ", " tmp"\n\t"\
    "pmaxsw  " tmp ", " dst"\n\t"

#define CHECK(pj, mj) \
    _movu " "#pj"(%[curr],%[mrefs]), " mm(2)"\n\t"\
    _movu " "#mj"(%[curr],%[prefs]), " mm(3)"\n\t"\
    _mova "    " mm(2)", " mm(4)"\n\t"\
    _mova "    " mm(2)", " mm(5)"\n\t"\
    "pxor      " mm(3)", " mm(4)"\n\t"\
    "pavgb     " mm(3)", " mm(5)"\n\t"\
    "pand      %[pb1],   " mm(4)"\n\t"\
    "psubusb   " mm(4)", " mm(5)"\n\t"\
    RSHIFT(      1,        mm(5))\
    "punpcklbw " mm(7)", " mm(5)"\n\t"\
    _mova "    " mm(2)", " mm(4)"\n\t"\
    "psubusb   " mm(3)", " mm(2)"\n\t"\
    "psubusb   " mm(4)", " mm(3)"\n\t"\
    "pmaxub    " mm(3)", " mm(2)"\n\t"\
    _mova "    " mm(2)", " mm(3)"\n\t"\
    _mova "    " mm(2)", " mm(4)"\n\t"\
    RSHIFT(      1,        mm(3))\
    RSHIFT(      2,        mm(4))\
    "punpcklbw " mm(7)", " mm(2)"\n\t"\
    "punpcklbw " mm(7)", " mm(3)"\n\t"\
    "punpcklbw " mm(7)", " mm(4)"\n\t"\
    "paddw     " mm(3)", " mm(2)"\n\t"\
    "paddw     " mm(4)", " mm(2)"\n\t"

#define CHECK1 \
    _mova "    " mm(0)", " mm(3)"\n\t"\
    "pcmpgtw   " mm(2)", " mm(3)"\n\t"\
    "pminsw    " mm(2)", " mm(0)"\n\t"\
    _mova "    " mm(3)", " mm(6)"\n\t"\
    "pand      " mm(3)", " mm(5)"\n\t"\
    "pandn     " mm(1)", " mm(3)"\n\t"\
    "por       " mm(5)", " mm(3)"\n\t"\
    _mova "    " mm(3)", " mm(1)"\n\t"

/* pretend not to have checked dir=2 if dir=1 was bad. hurts both quality and speed, but matches the C version. */
#define CHECK2 \
    "paddw     %[pw1],   " mm(6)"\n\t"\
    "psllw        $14,   " mm(6)"\n\t"\
    "paddsw    " mm(6)", " mm(2)"\n\t"\
    _mova "    " mm(0)", " mm(3)"\n\t"\
    "pcmpgtw   " mm(2)", " mm(3)"\n\t"\
    "pminsw    " mm(2)", " mm(0)"\n\t"\
    "pand      " mm(3)", " mm(5)"\n\t"\
    "pandn     " mm(1)", " mm(3)"\n\t"\
    "por       " mm(5)", " mm(3)"\n\t"\
    _mova "    " mm(3)", " mm(1)"\n\t"

#define FILTER(prev2, next2, alignment) \
    while (dest < destEnd)\
    {\
        asm volatile\
        (\
            "pxor      " mm(7)", " mm(7)"\n\t"\
            LOAD("(%[curr],%[mrefs])", mm(0))\
            LOAD("(%[curr],%[prefs])", mm(1))\
            LOAD("(%["#prev2"])",  mm(2))\
            LOAD("(%["#next2"])",  mm(3))\
            _mova "    " mm(3)", " mm(4)"\n\t"\
            "paddw     " mm(2)", " mm(3)"\n\t"\
            "psraw     $1,       " mm(3)"\n\t"\
            _mova "    " mm(0)", %[tmp0]\n\t"\
            _mova "    " mm(3)", %[tmp1]\n\t"\
            _mova "    " mm(1)", %[tmp2]\n\t"\
            "psubw     " mm(4)", " mm(2)"\n\t"\
            PABS(        mm(4),    mm(2))\
            LOAD("(%[prev],%[mrefs])", mm(3))\
            LOAD("(%[prev],%[prefs])", mm(4))\
            "psubw     " mm(0)", " mm(3)"\n\t"\
            "psubw     " mm(1)", " mm(4)"\n\t"\
            PABS(        mm(5),    mm(3))\
            PABS(        mm(5),    mm(4))\
            "paddw     " mm(4)", " mm(3)"\n\t"\
            "psrlw     $1,       " mm(2)"\n\t"\
            "psrlw     $1,       " mm(3)"\n\t"\
            "pmaxsw    " mm(3)", " mm(2)"\n\t"\
            LOAD("(%[next],%[mrefs])", mm(3))\
            LOAD("(%[next],%[prefs])", mm(4))\
            "psubw     " mm(0)", " mm(3)"\n\t"\
            "psubw     " mm(1)", " mm(4)"\n\t"\
            PABS(        mm(5),    mm(3))\
            PABS(        mm(5),    mm(4))\
            "paddw     " mm(4)", " mm(3)"\n\t"\
            "psrlw     $1,       " mm(3)"\n\t"\
            "pmaxsw    " mm(3)", " mm(2)"\n\t"\
            _mova "    " mm(2)", %[tmp3]\n\t"\
\
            "paddw     " mm(0)", " mm(1)"\n\t"\
            "paddw     " mm(0)", " mm(0)"\n\t"\
            "psubw     " mm(1)", " mm(0)"\n\t"\
            "psrlw     $1,       " mm(1)"\n\t"\
            PABS(        mm(2),    mm(0))\
\
            _movu " -1(%[curr],%[mrefs]), " mm(2)"\n\t"\
            _movu " -1(%[curr],%[prefs]), " mm(3)"\n\t"\
            _mova "             " mm(2)", " mm(4)"\n\t"\
            "psubusb            " mm(3)", " mm(2)"\n\t"\
            "psubusb            " mm(4)", " mm(3)"\n\t"\
            "pmaxub             " mm(3)", " mm(2)"\n\t"\
            SH(                   mm(2),    mm(3))\
            "punpcklbw          " mm(7)", " mm(2)"\n\t"\
            "punpcklbw          " mm(7)", " mm(3)"\n\t"\
            "paddw              " mm(2)", " mm(0)"\n\t"\
            "paddw              " mm(3)", " mm(0)"\n\t"\
            "psubw               %[pw1],  " mm(0)"\n\t"\
\
            CHECK(-2,0)\
            CHECK1\
            CHECK(-3,1)\
            CHECK2\
            CHECK(0,-2)\
            CHECK1\
            CHECK(1,-3)\
            CHECK2\
\
            /* Spatial check */ \
            _mova "    %[tmp3],  " mm(6)" \n\t"\
            "cmpl      $1, %[spatialCheck]\n\t"\
            "jne       1f\n\t"\
            LOAD("(%["#prev2"],%[mrefs],2)", mm(2))\
            LOAD("(%["#next2"],%[mrefs],2)", mm(4))\
            LOAD("(%["#prev2"],%[prefs],2)", mm(3))\
            LOAD("(%["#next2"],%[prefs],2)", mm(5))\
            "paddw     " mm(4)", " mm(2)"\n\t"\
            "paddw     " mm(5)", " mm(3)"\n\t"\
            "psrlw     $1,       " mm(2)"\n\t"\
            "psrlw     $1,       " mm(3)"\n\t"\
            _mova "    %[tmp0],  " mm(4)"\n\t"\
            _mova "    %[tmp1],  " mm(5)"\n\t"\
            _mova "    %[tmp2],  " mm(7)"\n\t"\
            "psubw     " mm(4)", " mm(2)"\n\t"\
            "psubw     " mm(7)", " mm(3)"\n\t"\
            _mova "    " mm(5)", " mm(0)"\n\t"\
            "psubw     " mm(4)", " mm(5)"\n\t"\
            "psubw     " mm(7)", " mm(0)"\n\t"\
            _mova "    " mm(2)", " mm(4)"\n\t"\
            "pminsw    " mm(3)", " mm(2)"\n\t"\
            "pmaxsw    " mm(4)", " mm(3)"\n\t"\
            "pmaxsw    " mm(5)", " mm(2)"\n\t"\
            "pminsw    " mm(5)", " mm(3)"\n\t"\
            "pmaxsw    " mm(0)", " mm(2)"\n\t"\
            "pminsw    " mm(0)", " mm(3)"\n\t"\
            "pxor      " mm(4)", " mm(4)"\n\t"\
            "pmaxsw    " mm(3)", " mm(6)"\n\t"\
            "psubw     " mm(2)", " mm(4)"\n\t"\
            "pmaxsw    " mm(4)", " mm(6)"\n\t"\
\
            "1:\n\t"\
            _mova "    %[tmp1],  " mm(2)"\n\t"\
            _mova "    " mm(2)", " mm(3)"\n\t"\
            "psubw     " mm(6)", " mm(2)"\n\t"\
            "paddw     " mm(6)", " mm(3)"\n\t"\
            "pmaxsw    " mm(2)", " mm(1)"\n\t"\
            "pminsw    " mm(3)", " mm(1)"\n\t"\
            "packuswb  " mm(1)", " mm(1)"\n\t"\
\
            _movh "    " mm(1)",  %[dest]\n\t"\
\
            :[tmp0]        "=m"(tmp0),\
             [tmp1]        "=m"(tmp1),\
             [tmp2]        "=m"(tmp2),\
             [tmp3]        "=m"(tmp3),\
             [dest]        "=m"(*dest)\
            :[prev]         "r"(prev),\
             [curr]         "r"(curr),\
             [next]         "r"(next),\
             [prefs]        "r"(prefs),\
             [mrefs]        "r"(mrefs),\
             [pw1]          "m"(pw_1),\
             [pb1]          "m"(pb_1),\
             [spatialCheck] "g"(spatialCheck)\
        );\
        dest  += alignment;\
        prev  += alignment;\
        curr  += alignment;\
        next  += alignment;\
    }

#ifdef QMPLAY2_CPU_X86_32 //Every x86-64 CPU has SSE2, so MMXEXT is unused there
static void filterLine_MMXEXT(quint8 *dest, const void *const destEnd,
                              const quint8 *prev, const quint8 *curr, const quint8 *next,
                              const qptrdiff prefs, const qptrdiff mrefs,
                              const int spatialCheck, const bool filterParity)
{
    const quint64 pw_1(0x0001000100010001ULL);
    const quint64 pb_1(0x0101010101010101ULL);
    quint64 tmp0, tmp1, tmp2, tmp3;

    #define _mova  "movq"
    #define _movu  "movq"
    #define _movh  "movd"
    #define  mm(n) "%%mm"#n

    #define RSHIFT(val, dst) \
        "psrlq   $"#val"*8, " dst"\n\t"

    #define SH(src, dst) \
        "pshufw $9, " src", " dst"\n\t"

    if (filterParity)
        FILTER(prev, curr, 4)
    else
        FILTER(curr, next, 4)

    asm volatile("emms");

    #undef SH
    #undef RSHIFT
    #undef mm
    #undef _movh
    #undef _movu
    #undef _mova
}
#endif // QMPLAY2_CPU_X86_32
static void filterLine_SSE2(quint8 *dest, const void *const destEnd,
                            const quint8 *prev, const quint8 *curr, const quint8 *next,
                            const qptrdiff prefs, const qptrdiff mrefs,
                            const int spatialCheck, const bool filterParity)
{
    struct u128
    {
        u128() = default;
        inline u128(quint64 v) : lo(v), hi(v) {}
        quint64 lo, hi;
    };
    __attribute__((aligned(16))) const u128 pw_1(0x0001000100010001ULL);
    __attribute__((aligned(16))) const u128 pb_1(0x0101010101010101ULL);
    __attribute__((aligned(16))) u128 tmp0, tmp1, tmp2, tmp3;

    #define _mova  "movdqa"
    #define _movu  "movdqu"
    #define _movh  "movq"
    #define  mm(n) "%%xmm"#n

    #define RSHIFT(val, dst) \
        "psrldq   $"#val", " dst"\n\t"

    #define SH(src, dst) \
        _mova " " src",    " dst"\n\t"\
        "psrldq   $2,      " dst"\n\t"

    if (filterParity)
        FILTER(prev, curr, 8)
    else
        FILTER(curr, next, 8)

    #undef SH
    #undef RSHIFT
    #undef mm
    #undef _movh
    #undef _movu
    #undef _mova
}

#undef LOAD
#undef PABS
#undef CHECK
#undef CHECK1
#undef CHECK2
#undef FILTER

#endif // QMPLAY2_CPU_X86
static void filterLine_CPP(quint8 *dest, const void *const destEnd,
                           const quint8 *prev, const quint8 *curr, const quint8 *next,
                           const qptrdiff prefs, const qptrdiff mrefs,
                           const int spatialCheck, const bool filterParity)
{
    filterLine<true>(dest, destEnd, prev, curr, next, prefs, mrefs, spatialCheck, filterParity);
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

    const int toSub = 3 + alignment - 1;

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
            filterLinePtr
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
    if (!filterLinePtr)
    {
        filterLinePtr = filterLine_CPP;
        alignment = 1;
#ifdef QMPLAY2_CPU_X86
        const int cpuFlags = QMPlay2CoreClass::getCPUFlags();
        if (cpuFlags & AV_CPU_FLAG_SSE2)
        {
            filterLinePtr = filterLine_SSE2;
            alignment = 8;
        }
#ifdef QMPLAY2_CPU_X86_32
        else if (cpuFlags & AV_CPU_FLAG_MMXEXT)
        {
            filterLinePtr = filterLine_MMXEXT;
            alignment = 4;
        }
#endif // QMPLAY2_CPU_X86_32
#endif // QMPLAY2_CPU_X86
    }
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
