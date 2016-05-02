#ifndef MAIN_HPP
#define MAIN_HPP

#define MUTEXWAIT_TIMEOUT 1250
#define TERMINATE_TIMEOUT (MUTEXWAIT_TIMEOUT*3)

#include <QMPlay2Core.hpp>

#include <QMap>

class QLocalServer;
class VideoDock;
class MenuBar;
class QWidget;

class QMPlay2GUIClass : private QMPlay2CoreClass
{
public:
	static QMPlay2GUIClass &instance();

	static QString getLongFromShortLanguage(const QString &);
	static QStringList getModules(const QString &, int typeLen);
	static QString getPipe();

	static void saveCover(QByteArray cover);

	static void drawPixmap(QPainter &p, QWidget *w, QPixmap pixmap);

#ifdef UPDATER
	void runUpdate(const QString &);
#endif

	QStringList getLanguages();
	void setLanguage();

	void setStyle();
	void loadIcons();

	QString getCurrentPth(QString pth = QString(), bool leaveFilename = false);
	void setCurrentPth(const QString &);

	void restoreGeometry(const QString &pth, QWidget *w, const QSize &def_size);

	inline QIcon getIcon(const QImage &img)
	{
		return img.isNull() ? *qmp2Pixmap : QPixmap::fromImage(img);
	}

	void updateInDockW();

	const QWidget *getVideoDock() const;

	QString langPath;

	QColor grad1, grad2, qmpTxt;
	QIcon *groupIcon, *mediaIcon, *folderIcon;

	MenuBar *menuBar;
	QWidget *mainW;
	QLocalServer *pipe;
	bool restartApp, removeSettings;
private:
	QMPlay2GUIClass();
	~QMPlay2GUIClass();

	void deleteIcons();
};

#define QMPlay2GUI QMPlay2GUIClass::instance()

#endif
