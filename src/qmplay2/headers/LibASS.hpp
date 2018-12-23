/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#pragma once

#include <QMPlay2Lib.hpp>

#include <QByteArray>
#include <QList>

class Settings;
class QMPlay2OSD;
struct ass_style;
struct ass_library;
struct ass_track;
struct ass_event;
struct ass_renderer;

class QMPLAY2SHAREDLIB_EXPORT LibASS
{
public:
#if defined Q_OS_WIN && !defined Q_OS_WIN64
	static bool slowFontCacheUpdate();
#endif

	static bool isDummy();

	LibASS(Settings &);
	~LibASS();

	void setWindowSize(int, int);
	void setARatio(double);
	void setZoom(double);
	void setFontScale(double);

	void addFont(const QByteArray &name, const QByteArray &data);
	void clearFonts();

	void initOSD();
	void setOSDStyle();
	bool getOSD(QMPlay2OSD *&, const QByteArray &, double);
	void closeOSD();

	void initASS(const QByteArray &header = QByteArray());
	bool isASS() const;
	void setASSStyle();
	void addASSEvent(const QByteArray &);
	void addASSEvent(const QByteArray &, double, double);
	void flushASSEvents();
	bool getASS(QMPlay2OSD *&, double);
	void closeASS();
private:
	void readStyle(const QString &, ass_style *);
	inline void calcSize();

	Settings &settings;

	ass_library *ass;
	int W, H, winW, winH;
	double zoom, aspect_ratio, fontScale;

	//OSD
	ass_track *osd_track;
	ass_style *osd_style;
	ass_event *osd_event;
	ass_renderer *osd_renderer;

	//ASS subtitles
	ass_track *ass_sub_track;
	ass_renderer *ass_sub_renderer;
	QList<ass_style *> ass_sub_styles_copy;
	bool hasASSData, overridePlayRes;
};
