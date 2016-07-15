#ifndef SCREENSAVER_HPP
#define SCREENSAVER_HPP

#include <QObject>

class ScreenSaverPriv;

class ScreenSaver : public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(ScreenSaver)
public:
	ScreenSaver();
	~ScreenSaver();

	bool isOk() const;

public slots:
	void reset();
private:
	ScreenSaverPriv *priv;
};

#endif // SCREENSAVER_HPP
