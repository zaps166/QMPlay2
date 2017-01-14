/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <VideoWriter.hpp>

#include <QWidget>
#include <QTimer>

#include <ddraw.h>

class DirectDrawWriter;
class QMPlay2OSD;

class Drawable : public QWidget
{
	Q_OBJECT
	typedef HRESULT (WINAPI *DwmEnableCompositionProc)(UINT uCompositionAction);
public:
	Drawable(DirectDrawWriter &);
	~Drawable();

	inline bool canDraw() const
	{
		return DDSSecondary;
	}

	void dock();
	bool createSecondary();
	void releaseSecondary();
	void videoEqSet();
	void setFlip();

	void draw(const VideoFrame &videoFrame);

	void resizeEvent(QResizeEvent *);

	QList<const QMPlay2OSD *> osd_list;
	QMutex osd_mutex;
	bool isOK, isOverlay, paused;
private:
	void getRects(RECT &, RECT &);
	void fillRects();

	Q_SLOT void updateOverlay();
	Q_SLOT void overlayVisible(bool);
	Q_SLOT void doOverlayVisible();
	void blit();

	bool restoreLostSurface();

	void paintEvent(QPaintEvent *);
	bool event(QEvent *);

	QPaintEngine *paintEngine() const;

	QImage osdImg;
	QList<QByteArray> osd_checksums;

	QTimer visibleTim;

	DirectDrawWriter &writer;

	int X, Y, W, H, flip;

	HBRUSH blackBrush;
	LPDIRECTDRAW DDraw;
	LPDIRECTDRAWCLIPPER DDClipper;
	LPDIRECTDRAWSURFACE DDSPrimary, DDSSecondary, DDSBackBuffer;
	LPDIRECTDRAWCOLORCONTROL DDrawColorCtrl;

	DwmEnableCompositionProc DwmEnableComposition;
};

/**/

class DirectDrawWriter : public VideoWriter
{
	friend class Drawable;
public:
	DirectDrawWriter(Module &);
private:
	~DirectDrawWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	void writeVideo(const VideoFrame &videoFrame);
	void writeOSD(const QList<const QMPlay2OSD *> &);

	void pause();

	QString name() const;

	bool open();

	/**/

	int outW, outH, flip, Hue, Saturation, Brightness, Contrast;
	double aspect_ratio, zoom;
	bool hasVideoSize;

	Drawable *drawable;
};

#define DirectDrawWriterName "DirectDraw"
