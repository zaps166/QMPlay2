#ifndef QMPLAY2CORE_HPP
#define QMPLAY2CORE_HPP

#include <QAtomicInt>
#include <QObject>
#include <QIcon>

#include <QDebug>

enum LogFlags {InfoLog = 0x1, ErrorLog = 0x2, SaveLog = 0x4, AddTimeToLog = 0x8, DontShowInGUI = 0x10, LogOnce = 0x20};

class Settings;
class QWidget;
class QPixmap;
class Module;

class QMPlay2CoreClass : public QObject
{
	Q_OBJECT
public:
	static inline QMPlay2CoreClass &instance()
	{
		return *qmplay2Core;
	}

	static bool canSuspend();
	static void suspend();

	void init(bool loadModules, const QString &_qmplay2Dir, const QString &_settingsDir = QString());
	void quit();

	inline QList<Module *> getPluginsInstance() const
	{
		return pluginsInstance;
	}

	inline QString getSettingsDir()
	{
		return settingsDir;
	}
	inline QString getQMPlay2Dir()
	{
		return qmplay2Dir;
	}

	inline QPixmap &getQMPlay2Pixmap()
	{
		return *qmp2Pixmap;
	}
	inline Settings &getSettings()
	{
		return *settings;
	}
	QIcon getIconFromTheme(const QString &icon);

	Q_SIGNAL void wallpaperChanged(bool hasWallpaper, double alpha);

	Q_SIGNAL void dockVideo(QWidget *w);

	Q_SIGNAL void showSettings(const QString &moduleName);

	Q_SIGNAL void videoDockMoved();
	Q_SIGNAL void mainWidgetNotMinimized(bool);
	Q_SIGNAL void videoDockVisible(bool);

	Q_SIGNAL void sendMessage(const QString &msg, const QString &title = QString(), int messageIcon = 1, int ms = 2000);
	Q_SIGNAL void processParam(const QString &param, const QString &data = QString());

	Q_SIGNAL void restoreCursor();
	Q_SIGNAL void waitCursor();
	Q_SIGNAL void busyCursor();

	Q_SIGNAL void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName);
	Q_SIGNAL void updateCover(const QString &title, const QString &artist, const QString &album, const QByteArray &cover);
	Q_SIGNAL void coverDataFromMediaFile(const QByteArray &cover);
	Q_SIGNAL void coverFile(const QString &filePath);

	Q_SIGNAL void playStateChanged(const QString &playState);
	Q_SIGNAL void fullScreenChanged(bool fs);
	Q_SIGNAL void speedChanged(double speed);
	Q_SIGNAL void volumeChanged(double vol);
	Q_SIGNAL void posChanged(int pos);
	Q_SIGNAL void seeked(int pos);

	bool run(const QString &command, const QString &args = QString());

	void log(const QString &, int logFlags = ErrorLog);
	inline void logInfo(const QString &txt, const bool showInGUI = true, const bool logOnce = false)
	{
		log(txt, InfoLog | SaveLog | AddTimeToLog | (showInGUI ? 0 : DontShowInGUI) | (logOnce ? LogOnce : 0));
	}
	inline void logError(const QString &txt, const bool showInGUI = true, const bool logOnce = false)
	{
		log(txt, ErrorLog | SaveLog | AddTimeToLog | (showInGUI ? 0 : DontShowInGUI) | (logOnce ? LogOnce : 0));
	}
	inline QString getLogFilePath()
	{
		return logFilePath;
	}

	inline void setWorking(bool w)
	{
		w ? working.ref() : working.deref();
	}
	inline bool isWorking()
	{
#if QT_VERSION < 0x050000
		return working > 0;
#else
		return working.load() > 0; //For Qt5 <= 5.2
#endif
	}

	virtual const QWidget *getVideoDock() const = 0;
private slots:
	void restoreCursorSlot();
	void waitCursorSlot();
	void busyCursorSlot();
protected:
	QMPlay2CoreClass();
	~QMPlay2CoreClass() {}

	QPixmap *qmp2Pixmap;
	Settings *settings;
private:
	Q_SIGNAL void logSignal(const QString &txt);

	static QMPlay2CoreClass *qmplay2Core;

	QList<Module *> pluginsInstance;
	QString qmplay2Dir, settingsDir, logFilePath;
#ifndef Q_OS_WIN
	QString unixOpenCommand;
#endif
	QAtomicInt working;
	QStringList logs;
};

#define QMPlay2Core QMPlay2CoreClass::instance()

#endif
