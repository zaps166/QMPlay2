#ifndef QMPLAY2MACEXTENSIONS_HPP
#define QMPLAY2MACEXTENSIONS_HPP

class QString;

namespace QMPlay2MacExtensions
{
	void init();
	void deinit();

	void setApplicationVisible(bool visible);

	void notify(const QString &title, const QString &msg, const int iconId, const int ms);
}

#endif // QMPLAY2MACEXTENSIONS_HPP
