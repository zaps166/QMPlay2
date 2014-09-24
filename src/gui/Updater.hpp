#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <QNetworkAccessManager>
#include <QDialog>
#include <QFile>

class QUrl;
class QLabel;
class QProgressBar;
class QPushButton;

class Updater : public QDialog
{
	Q_OBJECT
public:
	Updater( QWidget * );
	~Updater();

	inline bool downloading() const
	{
		return busy && updateFile.isOpen() && updateFile.size() > 0;
	}
public slots:
	void downloadUpdate();
private slots:
	void infoFinished();
	void headerChanged();
	void writeToFile();
	void downloadprogress( qint64, qint64 );
	void downloadFinished();

	void applyUpdate();
private:
	void getFile( const QUrl & );
	void endWork( const QString & );

	QFile infoFile, updateFile;
	QNetworkAccessManager net;
	QLabel *infoL;
	QProgressBar *progressB;
	QPushButton *downloadUpdateB, *installB;
	bool busy, firstChunk;
};

#endif //UPDATER_HPP
