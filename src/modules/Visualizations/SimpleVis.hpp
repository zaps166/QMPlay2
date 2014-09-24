#include <QMPlay2Extensions.hpp>
#include <VisWidget.hpp>

#include <QLinearGradient>
#include <QMutex>

class SimpleVis;

class SimpleVisW : public VisWidget
{
	friend class SimpleVis;
	Q_OBJECT
public:
	SimpleVisW( SimpleVis & );
private:
	void paintEvent( QPaintEvent * );
	void resizeEvent( QResizeEvent * );

	void start( bool v = false );
	void stop();

	QByteArray soundData;
	uchar chn;
	uint srate;
	int interval;
	float L, R, Lk, Rk;
	SimpleVis &simpleVis;
	QLinearGradient linearGrad;
	bool fullScreen;
};

/**/

class SimpleVis : public QMPlay2Extensions
{
public:
	SimpleVis( Module & );

	void soundBuffer( const bool );

	bool set();
private:
	DockWidget *getDockWidget();

	bool isVisualization() const;
	void connectDoubleClick( const QObject *, const char * );
	void visState( bool, uchar, uint );
	void sendSoundData( const QByteArray & );

	/**/

	SimpleVisW w;

	QByteArray tmpData;
	int tmpDataPos;
	QMutex mutex;
	float sndLen;
};

#define SimpleVisName "Prosta wizualizacja"
