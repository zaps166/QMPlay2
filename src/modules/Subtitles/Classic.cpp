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

#include <Classic.hpp>

#include <Functions.hpp>
#include <LibASS.hpp>

#include <QStringList>
#include <QRegExp>

#include <stdlib.h>
#include <stdio.h>

/**
 * TMP  - hh:mm:ss:napis - tu wiadomo, | oddziela linie
 * MPL2 - [start][koniec]napis - podane decysekundy, podzielić na 10.0 aby uzyskac sekundy, | oddziela linie
 * mDVD - {start}{koniec}napis - podane klatki filmu, należy przeliczyć na sekundy, używając FPS, | oddziela linie
**/

class SubWithoutEnd
{
public:
	inline SubWithoutEnd(unsigned start, double duration, const QByteArray &sub) :
		start(start), duration(duration), sub(sub)
	{}

	inline void setDuration(double duration)
	{
		if (duration < this->duration)
			this->duration = duration;
	}

	inline void operator +=(const SubWithoutEnd &other)
	{
		sub += '\n' + other.sub;
	}
	inline operator unsigned() const
	{
		return start;
	}

	unsigned start;
	double duration;
	QByteArray sub;
};

static inline void initOnce(bool &ok, LibASS *ass)
{
	if (!ok)
	{
		ass->initASS();
		ok = true;
	}
}

/**/

Classic::Classic(bool Use_mDVD_FPS, double Sub_max_s) :
	Use_mDVD_FPS(Use_mDVD_FPS), Sub_max_s(Sub_max_s)
{}

bool Classic::toASS(const QByteArray &txt, LibASS *ass, double fps)
{
	if (!ass)
		return false;

	bool ok = false, use_mDVD_FPS = Use_mDVD_FPS;

	const QRegExp TMPRegExp("\\d{1,2}:\\d{1,2}:\\d{1,2}\\D");
	const QRegExp MPL2RegExp("\\[\\d+\\]\\[\\d*\\]");
	const QRegExp MicroDVDRegExp("\\{\\d+\\}\\{\\d*\\}");

	QList<SubWithoutEnd> subsWithoutEnd;

	foreach (const QString &line, QString(txt).remove('\r').split('\n', QString::SkipEmptyParts))
	{
		double start = 0.0, duration = 0.0;
		QByteArray sub;
		int idx;

		if ((idx = line.indexOf(TMPRegExp)) > -1)
		{
			int h = -1, m = -1, s = -1;
			sscanf(line.toUtf8().data()+idx, "%d:%d:%d", &h, &m, &s);
			if (h > -1 && m > -1 && s > -1)
			{
				start = h*3600 + m*60 + s;
				sub = line.section(TMPRegExp, 1, 1, QString::SectionIncludeTrailingSep).toUtf8();
			}
		}
		else if ((idx = line.indexOf(MPL2RegExp)) > -1)
		{
			int s = -1, e = -1;
			sscanf(line.toUtf8().data()+idx, "[%d][%d]", &s, &e);
			if (s > -1)
			{
				start = s / 10.;
				duration = e / 10. - start;
				sub = line.section(MPL2RegExp, 1, 1, QString::SectionIncludeTrailingSep).toUtf8();
			}
		}
		else if ((idx = line.indexOf(MicroDVDRegExp)) > -1)
		{
			int s = -1, e = -1;
			sscanf(line.toUtf8().data()+idx, "{%d}{%d}", &s, &e);
			if (s > -1)
			{
				sub = line.section(MicroDVDRegExp, 1, 1, QString::SectionIncludeTrailingSep).toUtf8();
				if (use_mDVD_FPS && s == e && (s == 0 || s == 1))
				{
					use_mDVD_FPS = false;
					const double newFPS = atof(QByteArray(sub).replace('.', ',').data());
					if (newFPS > 0.0 && newFPS < 1000.0)
					{
						fps = newFPS;
						continue;
					}
				}
				start = s / fps;
				duration = e / fps - start;
			}
		}

		if (start >= 0.0 && !sub.isEmpty())
		{
			if (duration > 0.0)
			{
				initOnce(ok, ass);
				ass->addASSEvent(Functions::convertToASS(sub.replace('|', "\n")), start, duration);
			}
			else
				subsWithoutEnd.append(SubWithoutEnd(start, Sub_max_s, sub.replace('|', "\n")));
		}
	}

	if (!subsWithoutEnd.isEmpty())
	{
		qSort(subsWithoutEnd);

		for (int i = 0; i < subsWithoutEnd.size()-1; ++i)
		{
			const unsigned diff = subsWithoutEnd.at(i+1) - subsWithoutEnd.at(i);
			if (!diff)
			{
				subsWithoutEnd[i+1] += subsWithoutEnd.at(i);
				subsWithoutEnd.removeAt(i);
				--i;
			}
			else
				subsWithoutEnd[i].setDuration(diff);
		}

		initOnce(ok, ass);
		foreach (const SubWithoutEnd &sub, subsWithoutEnd)
			ass->addASSEvent(Functions::convertToASS(sub.sub), sub.start, sub.duration);
	}

	return ok;
}
