#include <VideoWriter.hpp>

qint64 VideoWriter::write(const QByteArray &)
{
	return -1;
}

bool VideoWriter::HWAccellInit(int W, int H, const char *codec_name)
{
	Q_UNUSED(W)
	Q_UNUSED(H)
	Q_UNUSED(codec_name)
	return false;
}
bool VideoWriter::HWAccellGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *yv12ToRGB32) const
{
	Q_UNUSED(videoFrame)
	Q_UNUSED(dest)
	Q_UNUSED(yv12ToRGB32)
	return false;
}
