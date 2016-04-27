#include <QMPlay2Extensions.hpp>
#include <IOController.hpp>

#include <QTreeWidget>
#include <QToolButton>
#include <QThread>

class QLabel;
class QGridLayout;
class QProgressBar;
class QTreeWidgetItem;
class DownloaderThread;

class DownloadItemW : public QWidget
{
	Q_OBJECT
public:
	DownloadItemW(DownloaderThread *downloaderThr, QString name, const QImage &img = QImage(), QDataStream *stream = NULL);
	~DownloadItemW();

	void setName(const QString &);
	void setSizeAndFilePath(qint64, const QString &);
	void setPos(int);
	void setSpeed(int);
	void finish(bool f = true);
	void error();

	inline QString getFilePath() const
	{
		return filePath;
	}

	inline bool isFinished() const
	{
		return finished;
	}

	inline void ssBEnable()
	{
		ssB->setEnabled(true);
	}

	void write(QDataStream &);

	bool dontDeleteDownloadThr;
signals:
	void start();
	void stop();
private slots:
	void toggleStartStop();
private:
	void downloadStop(bool);

	DownloaderThread *downloaderThr;

	QLabel *titleL, *sizeL, *iconL;
	QToolButton *ssB;

	class SpeedProgressWidget : public QWidget
	{
	public:
		QLabel *speedL;
		QProgressBar *progressB;
	} *speedProgressW;

	bool finished, readyToPlay;
	QString filePath;
};

/**/

class DownloadListW : public QTreeWidget
{
	friend class DownloaderW;
public:
	inline QString getDownloadsDirPath()
	{
		return downloadsDirPath;
	}
private:
	QString downloadsDirPath;
};

/**/

class DownloaderThread : public QThread
{
	Q_OBJECT
	enum {ADD_ENTRY, NAME, SET, SET_POS, SET_SPEED, DOWNLOAD_ERROR, FINISH};
public:
	DownloaderThread(QDataStream *, const QString &, DownloadListW *, const QString &name = QString(), const QString &prefix = QString(), const QString &param = QString());
	~DownloaderThread();

	void write(QDataStream &stream)
	{
		stream << url << prefix << param;
	}
signals:
	void listSig(int, qint64 val = 0, const QString &filePath = QString());
private slots:
	void listSlot(int, qint64, const QString &);
	void stop();
	void finished();
private:
	void run();

	QImage getImage();

	QString url, name, prefix, param;
	DownloadItemW *downloadItemW;
	DownloadListW *downloadLW;
	QTreeWidgetItem *item;
	IOController<> ioCtrl;
};

/**/

class DownloaderW : public QWidget
{
	Q_OBJECT
	friend class Downloader;
public:
	DownloaderW();
	~DownloaderW();
private slots:
	void setDownloadsDir();
	void clearFinished();
	void addUrl();
	void download();
	void itemDoubleClicked(QTreeWidgetItem *);
private:
	DockWidget *dw;

	QGridLayout *layout;
	DownloadListW *downloadLW;
	QToolButton *setDownloadsDirB, *clearFinishedB, *addUrlB;
};

/**/

class Downloader : public QMPlay2Extensions
{
public:
	inline Downloader(Module &module)
	{
		SetModule(module);
	}
	~Downloader();

	void init();

	DockWidget *getDockWidget();

	QAction *getAction(const QString &, double, const QString &, const QString &, const QString &);

	/**/

	DownloaderW *w;
};

#define DownloaderName "QMPlay2 Downloader"
