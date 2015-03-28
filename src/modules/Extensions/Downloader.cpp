#include <Downloader.hpp>

#include <Reader.hpp>
#include <Settings.hpp>
#include <Functions.hpp>
#include <QMPlay2Core.hpp>

#include <QTime>
#include <QTimer>
#include <QLabel>
#include <QAction>
#include <QMimeData>
#include <QClipboard>
#include <QFileDialog>
#include <QTreeWidget>
#include <QToolButton>
#include <QGridLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressBar>
#include <QApplication>
#ifdef Q_OS_WIN
	#include <QDesktopServices>
#endif

/**/

DownloadItemW::DownloadItemW( DownloaderThread *downloaderThr, QString name, const QImage &img, QDataStream *stream ) :
	dontDeleteDownloadThr( false ), downloaderThr( downloaderThr ), finished( false ), readyToPlay( false )
{
	QString sizeLText;

	if ( stream )
	{
		quint8 type;
		*stream >> filePath >> type >> name;
		finished = true;
		switch ( type )
		{
			case 1:
				readyToPlay = true;
				sizeLText = tr( "Pobieranie ukończone" );
				break;
			case 2:
				sizeLText = tr( "Pobieranie przerwane" );
				break;
			case 3:
				sizeLText = tr( "Błąd pobierania" );
				break;
		}
	}
	else
		sizeLText = tr( "Oczekiwanie na połączenie" );

	titleL = new QLabel;
	titleL->setText( name );

	sizeL = new QLabel;
	sizeL->setText( sizeLText );

	iconL = new QLabel;
	iconL->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred ) );
	iconL->setPixmap( img.isNull() ? QPixmap( ":/media" ) : QPixmap::fromImage( img ) );

	ssB = new QToolButton;
	if ( readyToPlay )
	{
		ssB->setIcon( QMPlay2Core.getIconFromTheme( "media-playback-start" ) );
		ssB->setToolTip( tr( "Odtwarzaj" ) );
	}
	else if ( finished )
	{
		ssB->setIcon( QMPlay2Core.getIconFromTheme( "view-refresh" ) );
		ssB->setToolTip( tr( "Pobierz ponownie" ) );
	}
	else
	{
		ssB->setIcon( QMPlay2Core.getIconFromTheme( "media-playback-stop" ) );
		ssB->setToolTip( tr( "Zatrzymaj pobieranie" ) );
	}
	connect( ssB, SIGNAL( clicked() ), this, SLOT( toggleStartStop() ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( iconL, 0, 0, 2, 1 );
	layout->addWidget( titleL, 0, 1, 1, 2 );
	layout->addWidget( sizeL, 1, 1, 1, 2 );
	if ( !finished )
	{
		QHBoxLayout *hLayout = new QHBoxLayout;

		speedProgressW = new SpeedProgressWidget;
		speedProgressW->setLayout( hLayout );

		speedProgressW->progressB = new QProgressBar;
		speedProgressW->progressB->setRange( 0, 0 );
		hLayout->addWidget( speedProgressW->progressB );

		speedProgressW->speedL = new QLabel;
		hLayout->addWidget( speedProgressW->speedL );

		layout->addWidget( speedProgressW, 2, 0, 1, 2 );
	}
	layout->addWidget( ssB, 2, 2, 1, 1 );
}
DownloadItemW::~DownloadItemW()
{
	if ( !dontDeleteDownloadThr )
	{
		finish( false );
		delete downloaderThr;
	}
}

void DownloadItemW::setName( const QString &name )
{
	if ( !finished )
		titleL->setText( name );
}
void DownloadItemW::setSizeAndFilePath( qint64 size, const QString &filePath )
{
	if ( !finished )
	{
		sizeL->setText( tr( "Rozmiar" ) + ": " + ( size > -1 ? Functions::sizeString( size ) : "?" ) );
		speedProgressW->progressB->setRange( 0, size > -1 ? 100 : 0 );
		this->filePath = filePath;
	}
}
void DownloadItemW::setPos( int pos )
{
	if ( !finished )
		speedProgressW->progressB->setValue( pos );
}
void DownloadItemW::setSpeed( int Bps )
{
	if ( !finished )
		speedProgressW->speedL->setText( Functions::sizeString( Bps ) + "/s" );
}
void DownloadItemW::finish( bool f )
{
	if ( !finished )
	{
		delete speedProgressW;
		if ( f )
			sizeL->setText( tr( "Pobieranie ukończone" ) );
		else
			sizeL->setText( tr( "Pobieranie przerwane" ) );
		downloadStop( f );
	}
}
void DownloadItemW::error()
{
	if ( !finished )
	{
		if ( speedProgressW->progressB->minimum() == speedProgressW->progressB->maximum() )
			speedProgressW->progressB->setRange( -1, 0 );
		speedProgressW->setEnabled( false );
		sizeL->setText( tr( "Błąd pobierania" ) );
		downloadStop( false );
	}
}

void DownloadItemW::write( QDataStream &stream )
{
	downloaderThr->write( stream );
	quint8 type = readyToPlay;
	if ( !type )
	{
		if ( sizeL->text() == tr( "Błąd pobierania" ) )
			type = 3;
		else
			type = 2;
	}
	stream << filePath << type << titleL->text();
}

void DownloadItemW::toggleStartStop()
{
	if ( readyToPlay )
	{
		if ( !filePath.isEmpty() )
			emit QMPlay2Core.processParam( "open", filePath );
		return;
	}
	if ( finished )
	{
		filePath.clear();
		emit start();
	}
	else
	{
		finish( false );
		ssB->setEnabled( false );
		emit stop();
	}
}

void DownloadItemW::downloadStop( bool ok )
{
	if ( !ok )
	{
		ssB->setIcon( QMPlay2Core.getIconFromTheme( "view-refresh" ) );
		ssB->setToolTip( tr( "Pobierz ponownie" ) );
	}
	else
	{
		ssB->setIcon( QMPlay2Core.getIconFromTheme( "media-playback-start" ) );
		ssB->setToolTip( tr( "Odtwarzaj" ) );
		readyToPlay = true;
	}
	finished = true;
	if ( !dontDeleteDownloadThr && visibleRegion() == QRegion() )
		emit QMPlay2Core.sendMessage( titleL->text(), sizeL->text() );
}

/**/

DownloaderThread::DownloaderThread( QDataStream *stream, const QString &url, DownloadListW *downloadLW, const QString &name, const QString &prefix, const QString &param ) :
	url( url ), name( name ), prefix( prefix ), param( param ), downloadItemW( NULL ), downloadLW( downloadLW ), item( NULL )
{
	connect( this, SIGNAL( listSig( int, qint64, const QString & ) ), this, SLOT( listSlot( int, qint64, const QString & ) ) );
	connect( this, SIGNAL( finished() ), this, SLOT( finished() ) );
	if ( stream )
	{
		*stream >> this->url >> this->prefix >> this->param;
		item = new QTreeWidgetItem( downloadLW );
		downloadLW->setItemWidget( item, 0, ( downloadItemW = new DownloadItemW( this, QString(), getImage(), stream ) ) );
		connect( downloadItemW, SIGNAL( start() ), this, SLOT( start() ) );
		connect( downloadItemW, SIGNAL( stop() ), this, SLOT( stop() ) );
	}
	else
		start();
}
DownloaderThread::~DownloaderThread()
{
	disconnect( this, SIGNAL( finished() ), this, SLOT( finished() ) );
	stop();
	if ( !wait( 2000 ) )
	{
		terminate();
		wait( 500 );
	}
}

void DownloaderThread::listSlot( int param, qint64 val, const QString &filePath )
{
	switch ( param )
	{
		case ADD_ENTRY:
			if ( !item )
				item = new QTreeWidgetItem( downloadLW );
			if ( downloadItemW )
			{
				downloadItemW->dontDeleteDownloadThr = true;
				delete downloadItemW;
			}
			downloadLW->setItemWidget( item, 0, ( downloadItemW = new DownloadItemW( this, name.isEmpty() ? url : name, getImage() ) ) );
			connect( downloadItemW, SIGNAL( start() ), this, SLOT( start() ) );
			connect( downloadItemW, SIGNAL( stop() ), this, SLOT( stop() ) );
			break;
		case NAME:
			downloadItemW->setName( name );
			break;
		case SET:
			downloadItemW->setSizeAndFilePath( val, filePath );
			break;
		case SET_POS:
			downloadItemW->setPos( val );
			break;
		case SET_SPEED:
			downloadItemW->setSpeed( val );
			break;
		case DOWNLOADERROR:
			downloadItemW->error();
			break;
		case FINISH:
			downloadItemW->finish();
			break;
	}
}
void DownloaderThread::stop()
{
	ioCtrl.abort();
}
void DownloaderThread::finished()
{
	if ( downloadItemW )
		downloadItemW->ssBEnable();
}

void DownloaderThread::run()
{
	ioCtrl.resetAbort();

	QString scheme = Functions::getUrlScheme( url );
	if ( scheme.isEmpty() )
		url.prepend( "http://" );
	else if ( scheme == "file" )
	{
		if ( !item )
			deleteLater();
		emit QMPlay2Core.sendMessage( tr( "Nieprawidłowy adres" ), DownloaderName );
		return;
	}

	emit listSig( ADD_ENTRY );

	QString newUrl = url;
	QString extension;

	if ( !prefix.isEmpty() )
		foreach ( QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList() )
			if ( QMPlay2Ext->addressPrefixList( false ).contains( prefix ) )
			{
				newUrl.clear();
				QMPlay2Ext->convertAddress( prefix, url, param, &newUrl, &name, NULL, &extension, &ioCtrl );
				break;
			}

	if ( name.isEmpty() )
	{
		name = Functions::fileName( newUrl );
		int idx = name.indexOf( "?" );
		if ( idx > -1 )
			name.remove( idx, name.size() - idx );
	}
	if ( !name.isEmpty() && !extension.isEmpty() )
		name += extension;

	emit listSig( NAME );

	if ( ioCtrl.isAborted() )
		return;

	bool err = true;

	QMPlay2Core.setWorking( true );

	IOController< Reader > &reader = ioCtrl.toRef< Reader >();
	if ( !newUrl.isEmpty() )
		Reader::create( newUrl, reader );
	if ( reader && reader->readyRead() && !reader->atEnd() )
	{
		QString filePath;
		quint16 num = 0;
		do
			filePath = downloadLW->getDownloadsDirPath() + ( num ? ( QString::number( num ) + "_" ) : QString() ) + Functions::cleanFileName( name );
		while ( QFile::exists( filePath ) && ++num < 0xFFFF );
		if ( num == 0xFFFF )
			filePath.clear();

		QFile file( filePath );
		if ( !file.fileName().isEmpty() && file.open( QFile::WriteOnly ) )
		{
			err = false;
			QTime speedT;
			int lastPos = -1;
			qint64 lastBytesPos = 0;

			emit listSig( SET, reader->size(), filePath );
			speedT.start();
			while ( !reader.isAborted() && !( err = !reader->readyRead() ) && !reader->atEnd() )
			{
				QByteArray arr = reader->read( 16384 );
				if ( arr.size() )
				{
					if ( file.write( arr ) != arr.size() )
					{
						err = true;
						break;
					}
				}
				else
				{
					if ( !reader.isAborted() && ( ( reader->size() < 0 && !file.size() ) || ( reader->size() > -1 && !reader->atEnd() ) ) )
						err = true;
					break;
				}

				const qint64 bytesPos = reader->pos();
				const int elapsed = speedT.elapsed();
				if ( elapsed >= 1000 )
				{
					emit listSig( SET_SPEED, ( bytesPos - lastBytesPos ) * 1000 / elapsed );
					lastBytesPos = bytesPos;
					speedT.restart();
				}
				if ( reader->size() > 0 )
				{
					const int pos = bytesPos * 100 / reader->size();
					if ( pos != lastPos )
					{
						emit listSig( SET_POS, pos );
						lastPos = pos;
					}
				}
			}
		}
		file.close();
		reader.clear();
	}
	emit listSig( err ? DOWNLOADERROR : FINISH );

	QMPlay2Core.setWorking( false );
}

QImage DownloaderThread::getImage()
{
	if ( !prefix.isEmpty() )
	{
		foreach ( QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList() )
		{
			QList< QMPlay2Extensions::AddressPrefix > addressPrefixList = QMPlay2Ext->addressPrefixList();
			int idx = addressPrefixList.indexOf( prefix );
			if ( idx > -1 )
				return addressPrefixList[ idx ].img;
		}
	}
	return QImage();
}

/**/

DownloaderW::DownloaderW()
{
	dw = new DockWidget;
	dw->setObjectName( DownloaderName );
	dw->setWindowTitle( tr( "Pobieranie plików z Internetu" ) );
	dw->setWidget( this );

	downloadLW = new DownloadListW;
	downloadLW->setHeaderHidden( true );
	downloadLW->setRootIsDecorated( false );
	connect( downloadLW, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ), this, SLOT( itemDoubleClicked( QTreeWidgetItem * ) ) );

	setDownloadsDirB = new QToolButton;
	setDownloadsDirB->setIcon( QMPlay2Core.getIconFromTheme( "folder-open" ) );
	setDownloadsDirB->setText( tr( "Katalog pobierania" ) );
	setDownloadsDirB->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
	setDownloadsDirB->setToolTip( tr( "Wybierz katalog dla pobieranych plików" ) );
	connect( setDownloadsDirB, SIGNAL( clicked() ), this, SLOT( setDownloadsDir() ) );

	clearFinishedB = new QToolButton;
	clearFinishedB->setIcon( QMPlay2Core.getIconFromTheme( "archive-remove" ) );
	clearFinishedB->setToolTip( tr( "Wyczyść zakończone pobierania" ) );
	connect( clearFinishedB, SIGNAL( clicked() ), this, SLOT( clearFinished() ) );

	addUrlB = new QToolButton;
	addUrlB->setIcon( QMPlay2Core.getIconFromTheme( "folder-new" ) );
	addUrlB->setToolTip( tr( "Wprowadź adres do pobrania" ) );
	connect( addUrlB, SIGNAL( clicked() ), this, SLOT( addUrl() ) );

	layout = new QGridLayout( this );
	layout->setMargin( 0 );
	layout->addWidget( downloadLW, 0, 0, 1, 4 );
	layout->addWidget( setDownloadsDirB, 1, 0, 1, 1 );
	layout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum ), 1, 1, 1, 1 );
	layout->addWidget( clearFinishedB, 1, 2, 1, 1 );
	layout->addWidget( addUrlB, 1, 3, 1, 1 );

	Settings sets( "Downloader" );
#ifdef Q_OS_WIN
	const QString defDownloadPath = QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation ).replace( '\\', '/' );
#else
	const QString defDownloadPath = QDir::homePath();
#endif
	downloadLW->downloadsDirPath = Functions::cleanPath( sets.getString( "DownloadsDirPath", defDownloadPath ) );

	const int count = sets.getInt( "Count" );
	if ( count > 0 )
	{
		const QByteArray arr = sets.getByteArray( "Items" );
		QDataStream stream( arr );
		for ( int i = 0 ; i < count ; ++i )
			new DownloaderThread( &stream, QString(), downloadLW );
		downloadLW->setCurrentItem( downloadLW->invisibleRootItem()->child( 0 ) );
	}
}
DownloaderW::~DownloaderW()
{
	int count = 0;
	QByteArray arr;
	QDataStream stream( &arr, QIODevice::WriteOnly );
	foreach ( QTreeWidgetItem *item, downloadLW->findItems( QString(), Qt::MatchContains ) )
	{
		DownloadItemW *downloadItemW = ( DownloadItemW * )downloadLW->itemWidget( item, 0 );
		downloadItemW->write( stream );
		++count;
	}
	Settings sets( "Downloader" );
	sets.set( "Count", count );
	sets.set( "Items", arr );
}

void DownloaderW::setDownloadsDir()
{
	QFileInfo dir( QFileDialog::getExistingDirectory( this, tr( "Wybierz katalog dla pobieranych plików" ), downloadLW->downloadsDirPath ) );
#ifndef Q_OS_WIN
	if ( dir.isDir() && dir.isWritable() )
#else
	if ( dir.isDir() )
#endif
	{
		downloadLW->downloadsDirPath = Functions::cleanPath( dir.filePath() );
		Settings( "Downloader" ).set( "DownloadsDirPath", downloadLW->downloadsDirPath );
	}
	else if ( dir.filePath() != QString() )
		QMessageBox::warning( this, DownloaderName, tr( "Nie udało się zmienić katalogu dla pobieranych plików" ) );
}
void DownloaderW::clearFinished()
{
	const QList< QTreeWidgetItem * > items = downloadLW->findItems( QString(), Qt::MatchContains );
	for ( int i = items.count() - 1 ; i >= 0 ; --i )
		if ( ( ( DownloadItemW * )downloadLW->itemWidget( items[ i ], 0 ) )->isFinished() )
			delete items[ i ];
}
void DownloaderW::addUrl()
{
	QString clipboardUrl;
	const QMimeData *mime = qApp->clipboard()->mimeData();
	if ( mime && mime->hasText() )
	{
		clipboardUrl = mime->text();
		if ( clipboardUrl.contains( '\n' ) || Functions::getUrlScheme( clipboardUrl ) != "http" )
			clipboardUrl.clear();
	}
	QString url = QInputDialog::getText( this, DownloaderName, tr( "Wprowadź adres" ), QLineEdit::Normal, clipboardUrl );
	if ( !url.isEmpty() )
		new DownloaderThread( NULL, url, downloadLW );
}
void DownloaderW::download()
{
	new DownloaderThread
	(
		NULL,
		sender()->property( "url" ).toString(),
		downloadLW,
		sender()->property( "name" ).toString(),
		sender()->property( "prefix" ).toString(),
		sender()->property( "param" ).toString()
	);
	downloadLW->setCurrentItem( downloadLW->invisibleRootItem()->child( 0 ) );
}
void DownloaderW::itemDoubleClicked( QTreeWidgetItem *item )
{
	DownloadItemW *downloadItemW = ( DownloadItemW * )downloadLW->itemWidget( item, 0 );
	if ( !downloadItemW->getFilePath().isEmpty() )
		emit QMPlay2Core.processParam( "open", downloadItemW->getFilePath() );
}

/**/

Downloader::~Downloader()
{
	delete w;
}

void Downloader::init()
{
	w = new DownloaderW;
}

DockWidget *Downloader::getDockWidget()
{
	return w->dw;
}

QAction *Downloader::getAction( const QString &name, int, const QString &url, const QString &prefix, const QString &param )
{
	if ( url.left( 7 ) == "file://" )
		return NULL;
	QAction *act = new QAction( DownloaderW::tr( "Pobierz" ), NULL );
	act->setIcon( QIcon( ":/downloader" ) );
	act->connect( act, SIGNAL( triggered() ), w, SLOT( download() ) );
	if ( !prefix.isEmpty() )
	{
		act->setProperty( "name", name );
		act->setProperty( "prefix", prefix );
		act->setProperty( "param", param );
	}
	act->setProperty( "url", url );
	return act;
}
