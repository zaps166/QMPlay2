#ifndef VIDEOWRITER_HPP
#define VIDEOWRITER_HPP

#include <Writer.hpp>

class QMPlay2_OSD;
class VideoFrame;
class ImgScaler;

class VideoWriter : public Writer
{
public:
	qint64 write(const QByteArray &);

	virtual void writeVideo(const VideoFrame &videoFrame) = 0;
	virtual void writeOSD(const QList<const QMPlay2_OSD *> &osd) = 0;

	virtual bool HWAccellInit(int W, int H, const char *codec_name);
	virtual bool HWAccellGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *yv12ToRGB32 = NULL) const;

	virtual bool open() = 0;
};

#endif //VIDEOWRITER_HPP
