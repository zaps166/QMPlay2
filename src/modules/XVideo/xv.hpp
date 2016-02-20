#include <QString>
#include <QMutex>
#include <QList>

class QRect;
class VideoFrame;
class QMPlay2_OSD;
struct XVideoPrivate;

class XVIDEO
{
public:
	XVIDEO();
	~XVIDEO();

	inline bool isOK()
	{
		return _isOK;
	}
	inline bool isOpen()
	{
		return _isOpen;
	}

	bool open(int, int, unsigned long, const QString &adaptorName, bool);
	void close();

	void draw(const VideoFrame &, const QRect &, const QRect &, int, int, const QList< const QMPlay2_OSD * > &, QMutex &);
	void redraw(const QRect &, const QRect &, int, int, int, int, int, int);

	void setVideoEqualizer(int, int, int, int);
	void setFlip(int);
	inline int flip()
	{
		return _flip;
	}

	static QList< QString > adaptorsList();
private:
	void putImage(const QRect &srcRect, const QRect &dstRect);

	void XvSetPortAttributeIfExists(void *attributes, int attrib_count, const char *k, int v);
	void clrVars();

	bool _isOK, _isOpen, hasImage, useSHM;
	int _flip;

	unsigned long handle;
	int width, height;

	unsigned int adaptors;

	QList< QByteArray > osd_checksums;

	XVideoPrivate *priv;
};
