#include <FFDec.hpp>

#include <QString>
#include <QList>

struct SwsContext;

class FFDecSW : public FFDec
{
public:
	FFDecSW( QMutex &, Module & );
private:
	~FFDecSW();

	bool set();

	QString name() const;

	int  decode( Packet &encodedPacket, QByteArray &decoded, bool flush, unsigned hurry_up );
	bool decodeSubtitle( const QByteArray &, double, double, QMPlay2_OSD *&, int, int );

	bool open( StreamInfo *, Writer * );

	/**/

	void getFromBitmapSubsBuffer( QMPlay2_OSD *&, double pts );

	int threads, lowres;
	bool respectHurryUP, skipFrames, forceSkipFrames, thread_type_slice;
	SwsContext *sws_ctx;

	struct BitmapSubBuffer
	{
		int x, y, w, h;
		double pts, duration;
		QByteArray bitmap;
	};
	QList< BitmapSubBuffer * > bitmapSubBuffer;
};
