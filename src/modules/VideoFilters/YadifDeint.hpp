#include <DeintFilter.hpp>

#include <QSharedPointer>
#include <QWaitCondition>
#include <QVector>
#include <QThread>
#include <QMutex>

class YadifDeint;

class YadifThr : public QThread
{
public:
	YadifThr(const YadifDeint &yadifDeint);
	~YadifThr();

	void start(VideoFrame &destFrame, const VideoFrame &prevFrame, const VideoFrame &currFrame, const VideoFrame &nextFrame, const int id, const int n);
	void waitForFinished();
private:
	void run();

	const YadifDeint &yadifDeint;

	VideoFrame *dest;
	const VideoFrame *prev, *curr, *next;
	int jobId, jobsCount;
	bool hasNewData, br;

	QWaitCondition cond;
	QMutex mutex;
};

class YadifDeint : public DeintFilter
{
	friend class YadifThr;
public:
	YadifDeint(bool doubler, bool spatialCheck);

	void filter(QQueue< FrameBuffer > &framesQueue);

	bool processParams(bool *paramsCorrected);
private:
	typedef QSharedPointer<YadifThr> YadifThrPtr;
	QVector<YadifThrPtr> threads;

	const bool doubler, spatialCheck;
	bool secondFrame;
	int w, h;
};

#define YadifDeintName "Yadif"
#define YadifNoSpatialDeintName YadifDeintName " (no spatial check)"
#define Yadif2xDeintName YadifDeintName " 2x"
#define Yadif2xNoSpatialDeintName Yadif2xDeintName " (no spatial check)"

#define YadifDescr "Yet Another DeInterlacong Filter"
