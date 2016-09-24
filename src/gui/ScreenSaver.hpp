#ifndef SCREENSAVER_HPP
#define SCREENSAVER_HPP

#include <QMap>

class ScreenSaverPriv;

class ScreenSaver
{
	Q_DISABLE_COPY(ScreenSaver)
public:
	ScreenSaver();
	~ScreenSaver();

	void inhibit(int context);
	void unInhibit(int context);

private:
	inline bool inhibitHelper(int context);
	inline bool unInhibitHelper(int context);

	ScreenSaverPriv *m_priv;

	typedef QMap<int, bool> Refs;
	Refs m_refs;
};

#endif // SCREENSAVER_HPP
