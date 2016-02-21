#include <FFDec.hpp>

#include <QString>
#include <QList>

struct SwsContext;

class FFDecSW : public FFDec
{
public:
	FFDecSW(QMutex &, Module &);
private:
	~FFDecSW();

	bool set();

	QString name() const;

	int  decodeAudio(Packet &encodedPacket, Buffer &decoded, bool flush);
	int  decodeVideo(Packet &encodedPacket, VideoFrame &decoded, bool flush, unsigned hurry_up);
	bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2_OSD *&osd, int w, int h);

	bool open(StreamInfo *, Writer *);

	/**/

	bool getFromBitmapSubsBuffer(QMPlay2_OSD *&, double pts);

	int threads, lowres;
	bool respectHurryUP, skipFrames, forceSkipFrames, thread_type_slice;
	int lastFrameW, lastFrameH, lastFrameFmt;
	SwsContext *sws_ctx;

	struct BitmapSubBuffer
	{
		int x, y, w, h;
		double pts, duration;
		QByteArray bitmap;
	};
	QList< BitmapSubBuffer * > bitmapSubBuffer;
};
