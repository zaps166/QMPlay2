#include <FFDec.hpp>

#include <QString>
#include <QList>

struct SwsContext;

class FFDecSW : public FFDec
{
public:
	FFDecSW(QMutex &, Module &);
private:
	class BitmapSubBuffer
	{
	public:
		inline BitmapSubBuffer(double pts, double duration) :
			pts(pts), duration(duration)
		{}
		inline BitmapSubBuffer(double pts) :
			x(0), y(0), w(0), h(0),
			pts(pts), duration(0.0)
		{}

		int x, y, w, h;
		double pts, duration;
		QByteArray bitmap;
	};

	~FFDecSW();

	bool set();

	QString name() const;

	void setSupportedPixelFormats(const QMPlay2PixelFormats &pixelFormats);

	int  decodeAudio(Packet &encodedPacket, Buffer &decoded, bool flush);
	int  decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up);
	bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2_OSD *&osd, int w, int h);

	bool open(StreamInfo &, Writer *);

	/**/

	void setPixelFormat();

	inline void addBitmapSubBuffer(BitmapSubBuffer *buff, double pos);
	bool getFromBitmapSubsBuffer(QMPlay2_OSD *&, double pts);
	inline void clearBitmapSubsBuffer();

	int threads, lowres;
	bool respectHurryUP, skipFrames, forceSkipFrames, thread_type_slice;
	int lastFrameW, lastFrameH, lastPixFmt;
	SwsContext *sws_ctx;

	QMPlay2PixelFormats supportedPixelFormats;
	quint8 chromaShiftW, chromaShiftH;
	int desiredPixFmt;
	bool dontConvert;

	QList<BitmapSubBuffer *> bitmapSubsBuffer;
};
