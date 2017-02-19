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

#include <QCoreApplication>

class OpenGL2Common;

class OpenGL2Writer : public VideoWriter
{
	Q_DECLARE_TR_FUNCTIONS(OpenGL2Writer)
public:
	OpenGL2Writer(Module &);
private:
	~OpenGL2Writer();

	bool set() override;

	bool readyWrite() const override final;

	bool processParams(bool *paramsCorrected) override;

	QMPlay2PixelFormats supportedPixelFormats() const override;

	void writeVideo(const VideoFrame &videoFrame) override;
	void writeOSD(const QList<const QMPlay2OSD *> &) override;

	void setHWAccelInterface(HWAccelInterface *hwAccelInterface) override;

	void pause() override;

	QString name() const override;

	bool open() override;

	/**/

	OpenGL2Common *drawable;
	bool allowPBO;
#ifdef OPENGL_NEW_API
	bool forceRtt, useRtt;
#endif
#ifdef VSYNC_SETTINGS
	bool vSync;
#endif
#ifdef Q_OS_WIN
	bool preventFullScreen;
#endif
};

#define OpenGL2WriterName "OpenGL 2"
