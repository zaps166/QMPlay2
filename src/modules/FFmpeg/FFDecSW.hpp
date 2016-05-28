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

	void setSupportedPixelFormats(const QMPlay2PixelFormats &pixelFormats);

	int  decodeAudio(Packet &encodedPacket, Buffer &decoded, bool flush);
	int  decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up);
	bool decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2_OSD *&osd, int w, int h);

	bool open(StreamInfo &, Writer *);

	/**/

	void setPixelFormat();

	bool getFromBitmapSubsBuffer(QMPlay2_OSD *&, double pts);

	int threads, lowres;
	bool respectHurryUP, skipFrames, forceSkipFrames, thread_type_slice;
	int lastFrameW, lastFrameH, lastPixFmt;
	SwsContext *sws_ctx;

	QMPlay2PixelFormats supportedPixelFormats;
	quint8 chromaShiftW, chromaShiftH;
	int desiredPixFmt;
	bool dontConvert;

	struct BitmapSubBuffer
	{
		int x, y, w, h;
		double pts, duration;
		QByteArray bitmap;
	};
	QList<BitmapSubBuffer *> bitmapSubBuffer;
};
