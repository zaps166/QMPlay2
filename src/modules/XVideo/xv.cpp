#include <xv.hpp>

#include <QMPlay2_OSD.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>

#include <QPainter>
#include <QX11Info>

#include <math.h>

#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>

struct XVideoPrivate
{
	XvImageFormatValues *fo;
	XvAdaptorInfo *ai;
	Display *display;
	XvImage *image;
	XvPortID port;
	GC gc;
	XShmSegmentInfo shmInfo;
	QImage osdImg;
};
#define fo priv->fo
#define ai priv->ai
#define disp priv->display
#define image priv->image
#define port priv->port
#define gc priv->gc
#define shmInfo priv->shmInfo
#define osdImg priv->osdImg

#define PutImage() { \
	if (!useSHM) \
		XvPutImage(disp, port, handle, gc, image, srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height()); \
	else \
		XvShmPutImage(disp, port, handle, gc, image, srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height(), false); \
}

XVIDEO::XVIDEO() :
	_isOK(false),
	_flip(0),
	priv(new XVideoPrivate)
{
	_flip = 0;
	ai = NULL;
	clrVars();
	_isOK = false;
	disp = QX11Info::display();
	if (!disp || XvQueryAdaptors(disp, DefaultRootWindow(disp), &adaptors, &ai) != Success)
		return;
	if (adaptors < 1)
		return;
	_isOK = true;
}
XVIDEO::~XVIDEO()
{
	close();
	if (ai)
		XvFreeAdaptorInfo(ai);
	delete priv;
}

bool XVIDEO::open(int W, int H, unsigned long _handle, const QString &adaptorName, bool _useSHM)
{
	if (isOpen())
		close();

	width = W;
	height = H;
	handle = _handle;
	useSHM = _useSHM;

	for (uint i = 0; i < adaptors; i++)
	{
		if ((ai[ i ].type & (XvInputMask | XvImageMask)) == (XvInputMask | XvImageMask))
		{
			if (!adaptorName.isEmpty() && adaptorName != ai[ i ].name)
				continue;
			for (XvPortID _p = ai[ i ].base_id; _p < ai[ i ].base_id + ai[ i ].num_ports; _p++)
			{
				if (!XvGrabPort(disp, _p, CurrentTime))
				{
					port = _p;
					break;
				}
			}
		}
		if (port)
			break;
	}
	if (!port)
	{
		close();
		return false;
	}

	int formats;
	fo = XvListImageFormats(disp, port, &formats);
	if (!fo)
	{
		close();
		return false;
	}

	int format_id = 0;
	for (int i = 0; i < formats; i++)
	{
		if (!qstrncmp(fo[ i ].guid, "YV12", 4))
		{
			format_id = fo[ i ].id;
			break;
		};
	};
	if (!format_id)
	{
		close();
		return false;
	}

	gc = XCreateGC(disp, handle, 0L, NULL);
	if (!gc)
	{
		close();
		return false;
	}

	if (!useSHM)
	{
		image = XvCreateImage(disp, port, format_id, NULL, width, height);
		if (!VideoFrame::testLinesize(width, image->pitches))
		{
			image->data = new char[ image->data_size ];
			mustCopy = true;
		}
	}
	else
	{
		image = XvShmCreateImage(disp, port, format_id, NULL, width, height, &shmInfo);
		shmInfo.shmid = shmget(IPC_PRIVATE, image->data_size, IPC_CREAT | 0777);
		if (shmInfo.shmid >= 0)
		{
			shmInfo.shmaddr = (char *)shmat(shmInfo.shmid, 0, 0);
			image->data = shmInfo.shmaddr;
			shmInfo.readOnly = false;
			if (!XShmAttach(disp, &shmInfo))
			{
				close();
				return false;
			}
			XSync(disp, false);
			shmctl(shmInfo.shmid, IPC_RMID, 0);
			mustCopy = true;
		}
		else
		{
			close();
			return false;
		}
	}

	if (!image)
	{
		close();
		return false;
	}

	osdImg = QImage(image->width, image->height, QImage::Format_ARGB32);
	osdImg.fill(0);

	_isOpen = true;
	return isOpen();
}
void XVIDEO::close()
{
	if (image)
	{
		if (useSHM && shmInfo.shmaddr)
		{
			XShmDetach(disp, &shmInfo);
			shmctl(shmInfo.shmid, IPC_RMID, 0);
			shmdt(shmInfo.shmaddr);
		}
		else if (!useSHM)
		{
			if (mustCopy)
				delete[] image->data;
			else
				VideoFrame::unref(videoFrameData);
		}
		XFree(image);
	}
	if (gc)
		XFreeGC(disp, gc);
	if (port)
		XvUngrabPort(disp, port, CurrentTime);
	if (fo)
		XFree(fo);
	clrVars();
}

void XVIDEO::draw(const QByteArray &_videoFrameData, const QRect &srcRect, const QRect &dstRect, int W, int H, const QList< const QMPlay2_OSD * > &osd_list, QMutex &osd_mutex)
{
	const int linesize = image->pitches[ 0 ];
	const int linesize1_2 = image->pitches[ 1 ];
	const int imageHeight = image->height;

	if (_flip & Qt::Horizontal)
		Functions::hFlip(*(char **)VideoFrame::fromData(_videoFrameData)->data, linesize, imageHeight, width);
	if (_flip & Qt::Vertical)
		Functions::vFlip(*(char **)VideoFrame::fromData(_videoFrameData)->data, linesize, imageHeight);

	if (mustCopy)
	{
		VideoFrame::copyYV12(image->data, _videoFrameData, linesize, linesize1_2, imageHeight);
		VideoFrame::unref(_videoFrameData);
	}
	else
	{
		VideoFrame::unref(videoFrameData);
		image->data = (char *)*VideoFrame::fromData(videoFrameData = _videoFrameData)->data;
	}

	osd_mutex.lock();
	if (!osd_list.isEmpty())
		Functions::paintOSDtoYV12((quint8 *)image->data, _videoFrameData, osdImg, W, H, linesize, linesize1_2, osd_list, osd_checksums);
	osd_mutex.unlock();

	PutImage();
	hasImage = true;
}
void XVIDEO::redraw(const QRect &srcRect, const QRect &dstRect, int X, int Y, int W, int H, int winW, int winH)
{
	if (isOpen())
	{
		if (Y > 0)
		{
			XFillRectangle(disp, handle, gc, 0, 0, winW, Y);
			XFillRectangle(disp, handle, gc, 0, Y + H, winW, Y+1);
		}
		if (X > 0)
		{
			XFillRectangle(disp, handle, gc, 0, 0, X, winH);
			XFillRectangle(disp, handle, gc, X + W, 0, X+1, winH);
		}
		if (hasImage)
			PutImage();
	}
}

void XVIDEO::setVideoEqualizer(int h, int s, int b, int c)
{
	if (isOpen())
	{
		int attrib_count;
		XvAttribute *attributes = XvQueryPortAttributes(disp, port, &attrib_count);
		if (attributes)
		{
			XvSetPortAttributeIfExists(attributes, attrib_count, "XV_HUE", h);
			XvSetPortAttributeIfExists(attributes, attrib_count, "XV_SATURATION", s);
			XvSetPortAttributeIfExists(attributes, attrib_count, "XV_BRIGHTNESS", b);
			XvSetPortAttributeIfExists(attributes, attrib_count, "XV_CONTRAST", c);
			XFree(attributes);
		}
	}
}
void XVIDEO::setFlip(int f)
{
	if (isOpen() && hasImage)
	{
		if ((f & Qt::Horizontal) != (_flip & Qt::Horizontal))
			Functions::hFlip(image->data, image->pitches[ 0 ], height, width);
		if ((f & Qt::Vertical) != (_flip & Qt::Vertical))
			Functions::vFlip(image->data, image->pitches[ 0 ], height);
	}
	_flip = f;
}

QList< QString > XVIDEO::adaptorsList()
{
	QList< QString > _adaptorsList;
	XVIDEO *xv = new XVIDEO;
	if  (xv->isOK())
	{
		for (unsigned i = 0; i < xv->adaptors; i++)
			if ((xv->ai[ i ].type & (XvInputMask | XvImageMask)) == (XvInputMask | XvImageMask))
				_adaptorsList += xv->ai[ i ].name;
	}
	delete xv;
	return _adaptorsList;
}

void XVIDEO::XvSetPortAttributeIfExists(void *attributes, int attrib_count, const char *k, int v)
{
	for (int i = 0; i < attrib_count; ++i)
	{
		const XvAttribute &attribute = ((XvAttribute *)attributes)[ i ];
		if (!qstrcmp(attribute.name, k) && (attribute.flags & XvSettable))
		{
			XvSetPortAttribute(disp, port, XInternAtom(disp, k, false), Functions::scaleEQValue(v, attribute.min_value, attribute.max_value));
			break;
		}
	}
}
void XVIDEO::clrVars()
{
	image = NULL;
	gc = NULL;
	port = 0;
	_isOpen = false;
	width = 0;
	height = 0;
	handle = 0;
	mustCopy = hasImage = false;
	fo = NULL;
	osdImg = QImage();
	osd_checksums.clear();
}
