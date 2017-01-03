/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <QMPlay2Extensions.hpp>
#include <VisWidget.hpp>

#include <QCoreApplication>
#include <QLinearGradient>

class SimpleVis;

class SimpleVisW : public VisWidget
{
	friend class SimpleVis;

	Q_DECLARE_TR_FUNCTIONS(SimpleVisW)
public:
	SimpleVisW(SimpleVis &);
private:
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *);

	void start(bool v = false, bool dontCheckRegion = false);
	void stop();

	QByteArray soundData;
	quint8 chn;
	quint32 srate;
	int interval;
	qreal leftBar, rightBar;
	QPair<qreal, double> leftLine, rightLine;
	SimpleVis &simpleVis;
	QLinearGradient linearGrad;
	bool fullScreen;
};

/**/

class SimpleVis : public QMPlay2Extensions
{
public:
	SimpleVis(Module &);

	void soundBuffer(const bool);

	bool set();
private:
	DockWidget *getDockWidget();

	bool isVisualization() const;
	void connectDoubleClick(const QObject *, const char *);
	void visState(bool, uchar, uint);
	void sendSoundData(const QByteArray &);
	void clearSoundData();

	/**/

	SimpleVisW w;

	QByteArray tmpData;
	int tmpDataPos;
	QMutex mutex;
	float sndLen;
};

#define SimpleVisName "Prosta wizualizacja"
