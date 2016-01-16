#ifndef CHAPTERINFO_HPP
#define CHAPTERINFO_HPP

#include <QString>

class ChapterInfo
{
public:
	inline ChapterInfo(double start, double end) :
		start(start), end(end)
	{}

	QString title;
	double start, end;
};

#endif // CHAPTERINFO_HPP
