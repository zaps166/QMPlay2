#ifndef HWACCELINTERFACE_HPP
#define HWACCELINTERFACE_HPP

#include <QString>

class VideoFrame;
class ImgScaler;

class HWAccelInterface
{
public:
	enum Field
	{
		FullFrame,
		TopField,
		BottomField
	};
	enum CopyResult
	{
		CopyNotReady = -1,
		CopyOk,
		CopyError,
	};

	virtual ~HWAccelInterface()
	{}

	virtual QString name() const = 0;

	virtual bool lock() = 0;
	virtual void unlock() = 0;

	virtual bool init(quint32 *textures) = 0;
	virtual void clear() = 0;

	virtual CopyResult copyFrame(const VideoFrame &videoFrame, Field field) = 0;

	virtual bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32 = NULL) = 0;
};

#endif // HWACCELINTERFACE_HPP
