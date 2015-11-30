#include <SimpleVis.hpp>

#include <QPainter>
#include <QMenu>

#include <math.h>

static inline void setValue( qreal &outV, qreal inV, int interval, qreal dz )
{
	if ( inV >= outV )
		outV = inV;
	else
		outV -= sqrt( outV ) * ( interval / dz );
}

static inline float fltclip( float f )
{
	if ( f > 1.0f )
		f = 1.0f;
	else if ( f < -1.0f )
		f = -1.0f;
	else if ( f != f ) //NaN
		f = 0.0f;
	return f;
}
static inline void fltcpy( float *dest, const float *src, int size )
{
	size /= sizeof( float );
	for ( int i = 0 ; i < size ; ++i )
		dest[ i ] = fltclip( src[ i ] );
}

/**/

SimpleVisW::SimpleVisW( SimpleVis &simpleVis ) :
	simpleVis( simpleVis ),
	fullScreen( false )
{
	dw->setObjectName( SimpleVisName );
	dw->setWindowTitle( tr( "Prosta wizualizacja" ) );
	dw->setWidget( this );

	chn = 2;
	srate = 0;
	L = R = Lk = Rk = 0.0;
	interval = -1;

	linearGrad.setStart( 0, 1 );
	linearGrad.setFinalStop( 0, 0 );
	linearGrad.setColorAt( 0.0, Qt::blue );
	linearGrad.setColorAt( 0.1, Qt::green );
	linearGrad.setColorAt( 0.5, Qt::yellow );
	linearGrad.setColorAt( 0.8, Qt::red );
}

void SimpleVisW::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	const int size = soundData.size() / sizeof( float );
	if ( size >= chn )
	{
		const float *samples = ( const float * )soundData.constData();

		qreal lr[ 2 ] = { 0.0f, 0.0f };

		p.translate( 0.0, fullScreen );
		p.scale( ( width() - 1 ) * 0.9, ( height() - 1 - fullScreen ) / 2.0 / chn );
		p.translate( 0.055, 0.0 );
		for ( int c = 0 ; c < chn ; ++c )
		{
			p.setPen( QPen( QColor( 102, 51, 128 ), 0.0 ) );
			p.drawLine( QLineF( 0.0, 1.0, 1.0, 1.0 ) );

			p.setPen( QPen( QColor( 102, 179, 102 ), 0.0 ) );
			QPainterPath path( QPointF( 0.0, 1.0-samples[ c ] ) );
			for ( int i = chn ; i < size ; i += chn )
				path.lineTo( i / ( qreal )( size - chn ), 1.0-samples[ i + c ] );
			p.drawPath( path );

			if ( c <= 2 )
			{
				const int numSamples = size / chn;
				for ( int i = 0 ; i < size ; i += chn )
					lr[ c ] += samples[ i + c ] * samples[ i + c ];
				lr[ c ] = sqrt( lr[ c ] / numSamples );
			}

			p.translate( 0.0, 2.0 );
		}

		p.resetTransform();
		p.scale( width()-1, height()-1 );

		if ( chn == 1 )
			lr[ 1 ] = lr[ 0 ];

		const int realInterval = timInterval.restart();

		/* Paski */
		::setValue( L, lr[ 0 ], realInterval, 500.0 );
		::setValue( R, lr[ 1 ], realInterval, 500.0 );
		p.fillRect( QRectF( 0.005, 1.0, 0.035, -L ), linearGrad );
		p.fillRect( QRectF( 0.960, 1.0, 0.035, -R ), linearGrad );

		/* Kreski */
		::setValue( Lk, lr[ 0 ], realInterval, 1500.0 );
		::setValue( Rk, lr[ 1 ], realInterval, 1500.0 );
		p.setPen( QPen( linearGrad, 0.0 ) );
		p.drawLine( QLineF( 0.005, -Lk+1.0, 0.040, -Lk+1.0 ) );
		p.drawLine( QLineF( 0.960, -Rk+1.0, 0.995, -Rk+1.0 ) );

		if ( stopped && tim.isActive() && Lk == lr[ 0 ] && Rk == lr[ 1 ] )
			tim.stop();
	}
}
void SimpleVisW::resizeEvent( QResizeEvent * )
{
	fullScreen = parentWidget() && parentWidget()->parentWidget() && parentWidget()->parentWidget()->isFullScreen();
}

void SimpleVisW::start( bool v )
{
	if ( v || dw->visibleRegion() != QRegion() || visibleRegion() != QRegion() )
	{
		simpleVis.soundBuffer( true );
		tim.start( interval );
		timInterval.start();
	}
}
void SimpleVisW::stop()
{
	tim.stop();
	simpleVis.soundBuffer( false );
	L = R = Lk = Rk = 0.0f;
}

/**/

SimpleVis::SimpleVis( Module &module ) :
	w( *this ), tmpDataPos( 0 )
{
	SetModule( module );
}

void SimpleVis::soundBuffer( const bool enable )
{
	QMutexLocker mL( &mutex );
	const int arrSize = enable ? ( ceil( sndLen * w.srate ) * w.chn * sizeof( float ) ) : 0;
	if ( arrSize != tmpData.size() || arrSize != w.soundData.size() )
	{
		tmpDataPos = 0;
		tmpData.clear();
		if ( arrSize )
		{
			tmpData.resize( arrSize );
			const int oldSize = w.soundData.size();
			w.soundData.resize( arrSize );
			if ( arrSize > oldSize )
				memset( w.soundData.data() + oldSize, 0, arrSize - oldSize );
		}
		else
			w.soundData.clear();
	}
}

bool SimpleVis::set()
{
	w.interval = sets().getInt( "RefreshTime" );
	sndLen = sets().getInt( "SimpleVis/SoundLength" ) / 1000.0f;
	if ( w.tim.isActive() )
		w.start();
	return true;
}

DockWidget *SimpleVis::getDockWidget()
{
	return w.dw;
}

bool SimpleVis::isVisualization() const
{
	return true;
}
void SimpleVis::connectDoubleClick( const QObject *receiver, const char *method )
{
	w.connect( &w, SIGNAL( doubleClicked() ), receiver, method );
}
void SimpleVis::visState( bool playing, uchar chn, uint srate )
{
	if ( playing )
	{
		if ( !chn || !srate )
			return;
		w.chn = chn;
		w.srate = srate;
		w.stopped = false;
		w.start();
	}
	else
	{
		if ( !chn && !srate )
		{
			w.srate = 0;
			w.stop();
		}
		w.stopped = true;
		w.update();
	}
}
void SimpleVis::sendSoundData( const QByteArray &data )
{
	if ( !w.tim.isActive() || !data.size() )
		return;
	QMutexLocker mL( &mutex );
	if ( !tmpData.size() )
		return;
	int newDataPos = 0;
	while ( newDataPos < data.size() )
	{
		const int size = qMin( data.size() - newDataPos, tmpData.size() - tmpDataPos );
		fltcpy( ( float * )( tmpData.data() + tmpDataPos ), ( const float * )( data.data() + newDataPos ), size );
		newDataPos += size;
		tmpDataPos += size;
		if ( tmpDataPos == tmpData.size() )
		{
			memcpy( w.soundData.data(), tmpData.constData(), tmpDataPos );
			tmpDataPos = 0;
		}
	}
}
