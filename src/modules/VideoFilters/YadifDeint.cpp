/*
 * This is the C++/Qt port of the Yadif deinterlacing filter from FFmpeg (libavfilter/vf_yadif.c)
 * Copyright (C) 2006-2011 Michael Niedermayer <michaelni@gmx.at>
 *
 * The Yadif MMXEXT implementation is taken from MPlayer project (libmpcodecs/vf_yadif.c)
 * Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>
*/

#include <YadifDeint.hpp>

#include <CPU.hpp>

extern "C"
{
	#include <libavutil/cpu.h>
}

static inline int max(int a, int b)
{
	return a > b ? a : b;
}
static inline int max3(int a, int b, int c)
{
	return max(max(a, b), c);
}
static inline int min(int a, int b)
{
	return a > b ? b : a;
}
static inline int min3(int a, int b, int c)
{
	return min(min(a, b), c);
}

static void (*filterLinePtr)(quint8 *, const void *const, const quint8 *, const quint8 *, const quint8 *, const quint8 *, const quint8 *, const qptrdiff, const qptrdiff, const bool);

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
		}
	}
}
template<bool isNotEdge>
static inline void filterLine(quint8 *dest, const void *const destEnd,
							  const quint8 *prev, const quint8 *curr, const quint8 *next, const quint8 *prev2, const quint8 *next2,
							  const qptrdiff prefs, const qptrdiff mrefs, const bool spatialCheck)
{
	while (dest != destEnd)
	{
		const int c = curr[mrefs];
		const int d = (prev2[0] + next2[0]) >> 1;
		const int e = curr[prefs];
		const int temporalDiff0 = abs(prev2[0] - next2[0]);
		const int temporalDiff1 = (abs(prev[mrefs] - c) + abs(prev[prefs] - e)) >> 1;
		const int temporalDiff2 = (abs(next[mrefs] - c) + abs(next[prefs] - e)) >> 1;

		int diff = max3(temporalDiff0 >> 1, temporalDiff1, temporalDiff2);
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
			const int maxVal = max3(d - e, d - c, min(b - c, f - e));
			const int minVal = min3(d - e, d - c, max(b - c, f - e));
			diff = max3(diff, minVal, -maxVal);
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
static void filterLine_MMXEXT(quint8 *dest, const void *const destEnd,
							  const quint8 *prev, const quint8 *curr, const quint8 *next, const quint8 *prev2, const quint8 *next2,
							  const qptrdiff prefs, const qptrdiff mrefs, const bool spatialCheck)
{
	static const quint64 pw_1 = 0x0001000100010001ULL;
	static const quint64 pb_1 = 0x0101010101010101ULL;
	quint64 tmp0, tmp1, tmp2, tmp3;

	#define LOAD4(mem,dst) \
		"movd      "mem", "#dst" \n\t"\
		"punpcklbw %%mm7, "#dst" \n\t"

	#define PABS(tmp,dst) \
		"pxor     "#tmp", "#tmp" \n\t"\
		"psubw    "#dst", "#tmp" \n\t"\
		"pmaxsw   "#tmp", "#dst" \n\t"

	#define CHECK(pj,mj) \
		"movq "#pj"(%[curr],%[mrefs]), %%mm2 \n\t"\
		"movq "#mj"(%[curr],%[prefs]), %%mm3 \n\t"\
		"movq      %%mm2, %%mm4 \n\t"\
		"movq      %%mm2, %%mm5 \n\t"\
		"pxor      %%mm3, %%mm4 \n\t"\
		"pavgb     %%mm3, %%mm5 \n\t"\
		"pand     %[pb1], %%mm4 \n\t"\
		"psubusb   %%mm4, %%mm5 \n\t"\
		"psrlq     $8,    %%mm5 \n\t"\
		"punpcklbw %%mm7, %%mm5 \n\t"\
		"movq      %%mm2, %%mm4 \n\t"\
		"psubusb   %%mm3, %%mm2 \n\t"\
		"psubusb   %%mm4, %%mm3 \n\t"\
		"pmaxub    %%mm3, %%mm2 \n\t"\
		"movq      %%mm2, %%mm3 \n\t"\
		"movq      %%mm2, %%mm4 \n\t"\
		"psrlq      $8,   %%mm3 \n\t"\
		"psrlq     $16,   %%mm4 \n\t"\
		"punpcklbw %%mm7, %%mm2 \n\t"\
		"punpcklbw %%mm7, %%mm3 \n\t"\
		"punpcklbw %%mm7, %%mm4 \n\t"\
		"paddw     %%mm3, %%mm2 \n\t"\
		"paddw     %%mm4, %%mm2 \n\t"

	#define CHECK1 \
		"movq      %%mm0, %%mm3 \n\t"\
		"pcmpgtw   %%mm2, %%mm3 \n\t"\
		"pminsw    %%mm2, %%mm0 \n\t"\
		"movq      %%mm3, %%mm6 \n\t"\
		"pand      %%mm3, %%mm5 \n\t"\
		"pandn     %%mm1, %%mm3 \n\t"\
		"por       %%mm5, %%mm3 \n\t"\
		"movq      %%mm3, %%mm1 \n\t"

	/* pretend not to have checked dir=2 if dir=1 was bad. hurts both quality and speed, but matches the C version. */
	#define CHECK2 \
		"paddw    %[pw1], %%mm6 \n\t"\
		"psllw     $14,   %%mm6 \n\t"\
		"paddsw    %%mm6, %%mm2 \n\t"\
		"movq      %%mm0, %%mm3 \n\t"\
		"pcmpgtw   %%mm2, %%mm3 \n\t"\
		"pminsw    %%mm2, %%mm0 \n\t"\
		"pand      %%mm3, %%mm5 \n\t"\
		"pandn     %%mm1, %%mm3 \n\t"\
		"por       %%mm5, %%mm3 \n\t"\
		"movq      %%mm3, %%mm1 \n\t"

	while (dest < destEnd)
	{
		asm volatile
		(
			"pxor      %%mm7, %%mm7 \n\t"
			LOAD4("(%[curr],%[mrefs])", %%mm0)
			LOAD4("(%[curr],%[prefs])", %%mm1)
			LOAD4("(%[prev2])", %%mm2)
			LOAD4("(%[next2])", %%mm3)
			"movq      %%mm3, %%mm4 \n\t"
			"paddw     %%mm2, %%mm3 \n\t"
			"psraw     $1,    %%mm3 \n\t"
			"movq      %%mm0, %[tmp0] \n\t"
			"movq      %%mm3, %[tmp1] \n\t"
			"movq      %%mm1, %[tmp2] \n\t"
			"psubw     %%mm4, %%mm2 \n\t"
			PABS(      %%mm4, %%mm2)
			LOAD4("(%[prev],%[mrefs])", %%mm3)
			LOAD4("(%[prev],%[prefs])", %%mm4)
			"psubw     %%mm0, %%mm3 \n\t"
			"psubw     %%mm1, %%mm4 \n\t"
			PABS(      %%mm5, %%mm3)
			PABS(      %%mm5, %%mm4)
			"paddw     %%mm4, %%mm3 \n\t"
			"psrlw     $1,    %%mm2 \n\t"
			"psrlw     $1,    %%mm3 \n\t"
			"pmaxsw    %%mm3, %%mm2 \n\t"
			LOAD4("(%[next],%[mrefs])", %%mm3)
			LOAD4("(%[next],%[prefs])", %%mm4)
			"psubw     %%mm0, %%mm3 \n\t"
			"psubw     %%mm1, %%mm4 \n\t"
			PABS(      %%mm5, %%mm3)
			PABS(      %%mm5, %%mm4)
			"paddw     %%mm4, %%mm3 \n\t"
			"psrlw     $1,    %%mm3 \n\t"
			"pmaxsw    %%mm3, %%mm2 \n\t"
			"movq      %%mm2, %[tmp3] \n\t"

			"paddw     %%mm0, %%mm1 \n\t"
			"paddw     %%mm0, %%mm0 \n\t"
			"psubw     %%mm1, %%mm0 \n\t"
			"psrlw     $1,    %%mm1 \n\t"
			PABS(      %%mm2, %%mm0)

			"movq -1(%[curr],%[mrefs]), %%mm2 \n\t"
			"movq -1(%[curr],%[prefs]), %%mm3 \n\t"
			"movq      %%mm2, %%mm4 \n\t"
			"psubusb   %%mm3, %%mm2 \n\t"
			"psubusb   %%mm4, %%mm3 \n\t"
			"pmaxub    %%mm3, %%mm2 \n\t"
			"pshufw $9,%%mm2, %%mm3 \n\t"
			"punpcklbw %%mm7, %%mm2 \n\t"
			"punpcklbw %%mm7, %%mm3 \n\t"
			"paddw     %%mm2, %%mm0 \n\t"
			"paddw     %%mm3, %%mm0 \n\t"
			"psubw    %[pw1], %%mm0 \n\t"

			CHECK(-2,0)
			CHECK1
			CHECK(-3,1)
			CHECK2
			CHECK(0,-2)
			CHECK1
			CHECK(1,-3)
			CHECK2

			"movq    %[tmp3], %%mm6 \n\t"
			"cmpb      $1, %[spatialCheck] \n\t"
			"jne       afterSpatialCheck \n\t"
			LOAD4("(%[prev2],%[mrefs],2)", %%mm2)
			LOAD4("(%[next2],%[mrefs],2)", %%mm4)
			LOAD4("(%[prev2],%[prefs],2)", %%mm3)
			LOAD4("(%[next2],%[prefs],2)", %%mm5)
			"paddw     %%mm4, %%mm2 \n\t"
			"paddw     %%mm5, %%mm3 \n\t"
			"psrlw     $1,    %%mm2 \n\t"
			"psrlw     $1,    %%mm3 \n\t"
			"movq    %[tmp0], %%mm4 \n\t"
			"movq    %[tmp1], %%mm5 \n\t"
			"movq    %[tmp2], %%mm7 \n\t"
			"psubw     %%mm4, %%mm2 \n\t"
			"psubw     %%mm7, %%mm3 \n\t"
			"movq      %%mm5, %%mm0 \n\t"
			"psubw     %%mm4, %%mm5 \n\t"
			"psubw     %%mm7, %%mm0 \n\t"
			"movq      %%mm2, %%mm4 \n\t"
			"pminsw    %%mm3, %%mm2 \n\t"
			"pmaxsw    %%mm4, %%mm3 \n\t"
			"pmaxsw    %%mm5, %%mm2 \n\t"
			"pminsw    %%mm5, %%mm3 \n\t"
			"pmaxsw    %%mm0, %%mm2 \n\t"
			"pminsw    %%mm0, %%mm3 \n\t"
			"pxor      %%mm4, %%mm4 \n\t"
			"pmaxsw    %%mm3, %%mm6 \n\t"
			"psubw     %%mm2, %%mm4 \n\t"
			"pmaxsw    %%mm4, %%mm6 \n\t"

			"afterSpatialCheck: \n\t"
			"movq    %[tmp1], %%mm2 \n\t"
			"movq      %%mm2, %%mm3 \n\t"
			"psubw     %%mm6, %%mm2 \n\t"
			"paddw     %%mm6, %%mm3 \n\t"
			"pmaxsw    %%mm2, %%mm1 \n\t"
			"pminsw    %%mm3, %%mm1 \n\t"
			"packuswb  %%mm1, %%mm1 \n\t"

			"movd %%mm1, %[dest]"

			:[tmp0] "=m"(tmp0),
			 [tmp1] "=m"(tmp1),
			 [tmp2] "=m"(tmp2),
			 [tmp3] "=m"(tmp3),
			 [dest] "=m"(*dest)
			:[prev]  "r"(prev),
			 [curr]  "r"(curr),
			 [next]  "r"(next),
			 [prev2] "r"(prev2),
			 [next2] "r"(next2),
			 [prefs] "r"(prefs),
			 [mrefs] "r"(mrefs),
			 [pw1]   "m"(pw_1),
			 [pb1]   "m"(pb_1),
			 [spatialCheck] "g"(spatialCheck)
		);
		dest  += 4;
		prev  += 4;
		curr  += 4;
		next  += 4;
		prev2 += 4;
		next2 += 4;
	}
	asm volatile("emms");
	#undef LOAD4
	#undef PABS
	#undef CHECK
	#undef CHECK1
	#undef CHECK2
	#undef FILTER
}
#endif // QMPLAY2_CPU_X86
static void filterLine_CPP(quint8 *dest, const void *const destEnd,
						   const quint8 *prev, const quint8 *curr, const quint8 *next, const quint8 *prev2, const quint8 *next2,
						   const qptrdiff prefs, const qptrdiff mrefs, const bool spatialCheck)
{
	filterLine<true>(dest, destEnd, prev, curr, next, prev2, next2, prefs, mrefs, spatialCheck);
}

static void filterSlice(const int plane, const int w, const int h, const int parity, const int tff, const bool spatialCheck,
						VideoFrame &destFrame, const VideoFrame &prevFrame, const VideoFrame &currFrame, const VideoFrame &nextFrame,
						const int jobId, const int jobsCount)
{
	const int sliceStart   = (h *  jobId   ) / jobsCount;
	const int sliceEnd     = (h * (jobId+1)) / jobsCount;
	const int refs         = currFrame.linesize[plane];
	const int destLinesize = destFrame.linesize[plane];
	const int filterParity = parity ^ tff;

	const quint8 *const prevData = prevFrame.buffer[plane].data();
	const quint8 *const currData = currFrame.buffer[plane].data();
	const quint8 *const nextData = nextFrame.buffer[plane].data();
	quint8 *const destData = destFrame.buffer[plane].data();

	for (int y = sliceStart; y < sliceEnd; ++y)
	{
		const quint8 *curr  = &currData[y * refs];
		quint8 *dest = &destData[y * destLinesize];
		if ((y ^ parity) & 1)
		{
			const quint8 *prev  = &prevData[y * refs];
			const quint8 *next  = &nextData[y * refs];
			const quint8 *prev2 = filterParity ? prev : curr;
			const quint8 *next2 = filterParity ? curr  : next;

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
				prev2,
				next2,
				prefs,
				mrefs,
				doSpatialCheck
			);
			filterLinePtr
			(
				dest  + 3,
				dest  - 3 + w,
				prev  + 3,
				curr  + 3,
				next  + 3,
				prev2 + 3,
				next2 + 3,
				prefs,
				mrefs,
				doSpatialCheck
			);
			filterLine<false>
			(
				dest  + w - 3,
				dest  + w,
				prev  + w - 3,
				curr  + w - 3,
				next  + w - 3,
				prev2 + w - 3,
				next2 + w - 3,
				prefs,
				mrefs,
				doSpatialCheck
			);
		}
		else
		{
			memcpy(dest, curr, w);
		}
	}
}

/* Yadif thread */

YadifThr::YadifThr(const YadifDeint &yadifDeint) :
	yadifDeint(yadifDeint),
	hasNewData(false), br(false)
{
	setObjectName("YadifThr");
	QThread::start();
}
YadifThr::~YadifThr()
{
	{
		QMutexLocker locker(&mutex);
		br = true;
		cond.wakeOne();
	}
	wait();
}

void YadifThr::start(VideoFrame &destFrame, const VideoFrame &prevFrame, const VideoFrame &currFrame, const VideoFrame &nextFrame, const int id, const int n)
{
	QMutexLocker locker(&mutex);
	dest = &destFrame;
	prev = &prevFrame;
	curr = &currFrame;
	next = &nextFrame;
	jobId = id;
	jobsCount = n;
	hasNewData = true;
	cond.wakeOne();
}
void YadifThr::waitForFinished()
{
	QMutexLocker locker(&mutex);
	while (hasNewData)
		cond.wait(&mutex);
}

void YadifThr::run()
{
	while (!br)
	{
		QMutexLocker locker(&mutex);
		if (!hasNewData && !br)
			cond.wait(&mutex);
		if (!hasNewData || br)
			continue;
		const bool tff = yadifDeint.isTopFieldFirst(*curr);
		for (int p = 0; p < 3; ++p)
		{
			const int shift  = (p ? 1 : 0);
			const int width  = yadifDeint.w >> shift;
			const int height = yadifDeint.h >> shift;
			filterSlice
			(
				p,
				width, height,
				yadifDeint.secondFrame == tff, tff,
				yadifDeint.spatialCheck,
				*dest, *prev, *curr, *next,
				jobId, jobsCount
			);
		}
		hasNewData = false;
		cond.wakeOne();
	}
}

/* Yadif deint filter */

YadifDeint::YadifDeint(bool doubler, bool spatialCheck) :
	doubler(doubler),
	spatialCheck(spatialCheck)
{
	if (!filterLinePtr)
	{
		filterLinePtr = filterLine_CPP;
#ifdef QMPLAY2_CPU_X86
		const int cpuFlags = av_get_cpu_flags();
		if (cpuFlags & AV_CPU_FLAG_MMXEXT)
			filterLinePtr = filterLine_MMXEXT; // I hope that the linesize is always divisible at least by 4 :)
#endif
	}
	addParam("W");
	addParam("H");
}

bool YadifDeint::filter(QQueue< FrameBuffer > &framesQueue)
{
	int insertAt = addFramesToDeinterlace(framesQueue);
	if (internalQueue.count() >= 3)
	{
		const FrameBuffer &prevBuffer = internalQueue.at(0);
		const FrameBuffer &currBuffer = internalQueue.at(1);
		const FrameBuffer &nextBuffer = internalQueue.at(2);

		VideoFrame destFrame(h, (h + 1) >> 1, currBuffer.frame.linesize);

		for (int i = 0; i < threads.count(); ++i)
			threads[i]->start(destFrame, prevBuffer.frame, currBuffer.frame, nextBuffer.frame, i, threads.count());
		for (int i = 0; i < threads.count(); ++i)
			threads[i]->waitForFinished();

		double ts = currBuffer.ts;
		if (secondFrame)
			ts += halfDelay(internalQueue.at(2), currBuffer);
		framesQueue.insert(insertAt++, FrameBuffer(destFrame, ts));

		if (secondFrame || !doubler)
			internalQueue.removeFirst();
		if (doubler)
			secondFrame = !secondFrame;
	}
	return internalQueue.count() >= 3;
}

bool YadifDeint::processParams(bool *)
{
	w = getParam("W").toInt();
	h = getParam("H").toInt();
	deintFlags = getParam("DeinterlaceFlags").toInt();
	if (w < 3 || h < 3 || (doubler == !(deintFlags & DoubleFramerate)))
		return false;
	if (threads.isEmpty())
	{
		threads.resize(QThread::idealThreadCount());
		for (int i = 0; i < threads.count(); ++i)
			threads[i] = YadifThrPtr(new YadifThr(*this));
	}
	secondFrame = false;
	return true;
}
