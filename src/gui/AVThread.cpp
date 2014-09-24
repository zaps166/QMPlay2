#include <AVThread.hpp>

#include <Main.hpp>
#include <PlayClass.hpp>

#include <Writer.hpp>
#include <Decoder.hpp>

AVThread::AVThread( PlayClass &playC, const QString &writer_type, Writer *_writer, const QStringList &pluginsName ) :
	dec( NULL ), playC( playC ), br( false ), br2( false ), waiting( false )
{
	connect( this, SIGNAL( finished() ), this, SLOT( deleteLater() ) );

	if ( _writer )
		writer = _writer;
	else
		writer = Writer::create( writer_type, pluginsName );

	mutex.lock();

	if ( writer )
		start();
}
AVThread::~AVThread()
{
	delete dec;
	delete writer;
}

void AVThread::destroyDec()
{
	delete dec;
	dec = NULL;
}

bool AVThread::lock()
{
	br2 = true;
	if ( !mutex.tryLock( MUTEXWAIT_TIMEOUT ) )
	{
		emit QMPlay2Core.waitCursor();
		bool ret = mutex.tryLock( MUTEXWAIT_TIMEOUT );
		emit QMPlay2Core.restoreCursor();
		return ret;
	}
	return true;
}

void AVThread::stop( bool _terminate )
{
	if ( _terminate )
		return terminate();

	br = br2 = true;
	mutex.unlock();
	playC.emptyBufferCond.wakeAll();

	if ( !wait( TERMINATE_TIMEOUT ) )
		terminate();
}

void AVThread::terminate()
{
	disconnect( this, SIGNAL( finished() ), this, SLOT( deleteLater() ) );
	QThread::terminate();
	delete this;
}
