#include <QTextEdit>

#include <DockWidget.hpp>

class TextEdit : public QTextEdit
{
	Q_OBJECT
private:
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *);
};

/**/

class QGridLayout;
class QLabel;

class InfoDock : public DockWidget
{
	friend class TextEdit;
	Q_OBJECT
public:
	InfoDock();
public slots:
	void setInfo(const QString &, bool, bool);
	void updateBitrate(int, int, double);
	void updateBuffered(qint64 backwardBytes, qint64 remainingBytes, double backwardSeconds, double remainingSeconds);
	void clear();
	void visibilityChanged(bool);
private:
	void setBRLabels();
	void setBufferLabel();

	QWidget mainW;
	QGridLayout *layout;
	QLabel *bitrate, *buffer;
	TextEdit *infoE;

	bool videoPlaying, audioPlaying;
	int audioBR, videoBR;
	double videoFPS;
	qint64 bytes1, bytes2;
	double seconds1, seconds2;
signals:
	void seek(int);
	void chStream(const QString &);
	void saveCover();
};
