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

#include <SRT.hpp>
#include <Functions.hpp>
#include <LibASS.hpp>

#include <QStringList>
#include <QRegExp>

#include <stdio.h>

bool SRT::toASS(const QByteArray &srt, LibASS *ass, double)
{
	if (!ass)
		return false;

	QRegExp colorRegExp("<font\\s+color\\s*=\\s*\\\"?\\#?(\\w{6})\\\"?>(.*)</font>", Qt::CaseInsensitive);
	colorRegExp.setMinimal(true);

	bool ok = false;
	const char *scanfFmt = (srt.left(11 /* Including BOM */).contains("WEBVTT")) ? "%d:%d:%d.%d" : "%d:%d:%d,%d";

	foreach (const QString &entry, QString(QString("\n\n") + srt).remove('\r').split(QRegExp("\n\n+(\\d+\n)?"), QString::SkipEmptyParts))
	{
		int idx = entry.indexOf('\n');
		if (idx > -1)
		{
			const QStringList time = entry.mid(0, idx).split(" --> ");
			if (time.size() == 2)
			{
				double time_double[2] = {-1.0, -1.0};
				for (int i = 0; i < 2; ++i)
				{
					int h = -1, m = -1, s = -1, ms = -1;
					sscanf(time[i].toLatin1().data(), scanfFmt, &h, &m, &s, &ms);
					if (h > -1 && m > -1 && s > -1 && ms > -1)
						time_double[i] = h*3600 + m*60 + s + ms/1000.0;
					else
						break;
				}
				if (time_double[0] >= 0.0 && time_double[1] > time_double[0])
				{
					if (!ok)
					{
						ass->initASS();
						ok = true;
					}

					QString txt = Functions::convertToASS(entry.mid(idx+1));

					//Colors
					int pos = 0;
					while ((pos = colorRegExp.indexIn(txt, pos)) != -1)
					{
						QString rgb = colorRegExp.cap(1);
						rgb = rgb.mid(4, 2) + rgb.mid(2, 2) + rgb.mid(0, 2);

						const QString replaced = "{\\1c&" + rgb + "&}" + colorRegExp.cap(2) + "{\\1c}";

						txt.replace(pos, colorRegExp.matchedLength(), replaced);
						pos += replaced.length();
					}

					ass->addASSEvent(txt.toUtf8(), time_double[0], time_double[1]-time_double[0]);
				}
			}
		}
	}

	return ok;
}
