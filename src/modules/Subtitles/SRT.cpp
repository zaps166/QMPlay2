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

	bool ok = false;
	const char *scanfFmt = (srt.left(11 /* Including BOM */).contains("WEBVTT")) ? "%d:%d:%d.%d" : "%d:%d:%d,%d";

	foreach (const QString &entry, QString(QString("\n\n") + srt).remove('\r').split(QRegExp("\n\n+(\\d+\n)?"), QString::SkipEmptyParts))
	{
		int idx = entry.indexOf('\n');
		if (idx > -1)
		{
			QStringList time = entry.mid(0, idx).split(" --> ");
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
					ass->addASSEvent(Functions::convertToASS(entry.mid(idx+1).toUtf8()), time_double[0], time_double[1]-time_double[0]);
				}
			}
		}
	}

	return ok;
}
