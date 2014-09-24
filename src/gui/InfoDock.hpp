#include <QTextEdit>

#include <DockWidget.hpp>

class TextEdit : public QTextEdit
{
	Q_OBJECT
private:
	void mouseMoveEvent( QMouseEvent * );
	void mousePressEvent( QMouseEvent * );
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
	void setInfo( const QString &, bool, bool );
	void updateBitrate( int, int, double );
	void updateBuffered( qint64, double );
	void clear();
	void visibilityChanged( bool );
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
	qint64 bs;
	double bt;
signals:
	void seek( int );
	void chStream( const QString & );
	void saveCover();
};
