#ifndef SCREENSAVER_HPP
#define SCREENSAVER_HPP

#include <QObject>

class ScreenSaverPriv;

class ScreenSaver : public QObject
{
	Q_DISABLE_COPY(ScreenSaver)
public:
	ScreenSaver();
	~ScreenSaver();

	void block();
	void unblock();

private:
	ScreenSaverPriv *m_priv;
	qint32 m_ref;
};

#endif // SCREENSAVER_HPP
