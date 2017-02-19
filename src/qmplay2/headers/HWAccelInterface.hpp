#ifndef HWACCELINTERFACE_HPP
#define HWACCELINTERFACE_HPP

#include <VideoAdjustment.hpp>

#include <QString>

class VideoFrame;
class ImgScaler;

class HWAccelInterface
{
public:
	enum Format
	{
		NV12,
		RGB32
	};
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

	virtual Format getFormat() const = 0;

	virtual bool lock()
	{
		return true;
	}
	virtual void unlock()
	{}

	virtual bool init(quint32 *textures) = 0;
	virtual void clear(bool contextChange) = 0;

	virtual CopyResult copyFrame(const VideoFrame &videoFrame, Field field) = 0;

	virtual bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32 = nullptr) = 0;

	virtual void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap)
	{
		videoAdjustmentCap.zero();
	}
	virtual void setVideAdjustment(const VideoAdjustment &videoAdjustment)
	{
		Q_UNUSED(videoAdjustment)
	}
};

#endif // HWACCELINTERFACE_HPP
