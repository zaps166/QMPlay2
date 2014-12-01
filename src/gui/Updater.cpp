#include <Updater.hpp>

#include <Settings.hpp>
#include <Main.hpp>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSettings>
#include <QLabel>
#include <QDate>
#include <QDir>

static QDate ParseVersion( const QString &dateTxt )
{
	QStringList l = dateTxt.split( '.' );
	int y = 0, m = 0, d = 0;
	if ( l.count() == 3 )
	{
		y = l[ 0 ].toInt() + 2000;
		m = l[ 1 ].toInt();
		d = l[ 2 ].toInt();
	}
	if ( y < 2000 || m < 1 || m > 12 || d < 1 || d > 31 )
		y = m = d = 0;
	return QDate( y, m, d );
}

/**/

Updater::Updater( QWidget *parent ) :
	QDialog( parent ),
	net( this ),
	busy( false )
{
	setWindowTitle( tr( "Aktualizacje programu QMPlay2" ) );
	infoFile.setFileName( QDir::tempPath() + "/QMPlay2_download_info." + QString::number( qrand() ) );
	updateFile.setFileName( QMPlay2Core.getSettingsDir() + "QMPlay2Installer" );
#ifdef Q_OS_WIN
	updateFile.setFileName( updateFile.fileName() + ".exe" );
#endif

	infoL = new QLabel( windowTitle() );
	progressB = new QProgressBar;
	downloadUpdateB = new QPushButton( tr( "Pobierz aktualizacje" ) );
	installB = new QPushButton( tr( "Zastosuj aktualizację" ) );
	connect( downloadUpdateB, SIGNAL( clicked() ), this, SLOT( downloadUpdate() ) );
	connect( installB, SIGNAL( clicked() ), this, SLOT( applyUpdate() ) );

	QVBoxLayout *layout = new QVBoxLayout( this );
	layout->setMargin( 2 );
	layout->addWidget( infoL );
	layout->addWidget( downloadUpdateB );
	layout->addWidget( progressB );
	layout->addWidget( installB );

	progressB->hide();
	installB->hide();

	resize( 350, 0 );
}
Updater::~Updater()
{
	if ( busy )
	{
		infoFile.remove();
		updateFile.remove();
	}
}

void Updater::downloadUpdate()
{
	if ( !busy && infoFile.open( QFile::WriteOnly | QFile::Truncate ) )
	{
		QNetworkRequest request( QUrl( "http://zaps166.sourceforge.net/?dwn=qmplay2/QMPlay2_download_info" ) );
		request.setRawHeader( "User-Agent", "QMPlay2" );
		QNetworkReply *reply = net.get( request );
		connect( reply, SIGNAL( finished() ), this, SLOT( infoFinished() ) );
		infoL->setText( tr( "Sprawdzanie dostępności aktualizacji" ) );
		progressB->setRange( 0, 0 );
		downloadUpdateB->hide();
		progressB->show();
		busy = true;
	}
}

void Updater::infoFinished()
{
	QNetworkReply *reply = ( QNetworkReply * )sender();
	if ( !reply->error() )
	{
		infoFile.write( reply->readAll() );
		infoFile.close();

		QSettings info( infoFile.fileName(), QSettings::IniFormat, this );

		QString ThisVersion = QMPlay2Core.getSettings().getString( "Version" );
		QString NewVersion  = info.value( "Version" ).toString();

		QDate NewVersionDate = ParseVersion( NewVersion );
		if ( NewVersionDate.isValid() )
		{
			if ( NewVersionDate > ParseVersion( ThisVersion ) )
			{
				QString FileURL;
#if defined Q_OS_WIN && defined QMPLAY2_CPU_X86
				if ( sizeof( void * ) == 8 )
					FileURL = info.value( "Win64" ).toString();
				if ( FileURL.isEmpty() ) //32bit or 64bit does not exists on server
					FileURL = info.value( "Win32" ).toString();
#elif defined Q_OS_LINUX && defined QMPLAY2_CPU_X86
				if ( sizeof( void * ) == 8 )
					FileURL = info.value( "Linux64" ).toString();
				if ( FileURL.isEmpty() ) //32bit or 64bit does not exists on server
					FileURL = info.value( "Linux32" ).toString();
#endif
				if ( FileURL.isEmpty() )
					endWork( tr( "Brak dostępnej aktualizacji" ) );
				else if ( updateFile.open( QFile::WriteOnly | QFile::Truncate ) )
					getFile( QUrl( FileURL.replace( "$Version", NewVersion ) ) );
				else
					endWork( tr( "Błąd zapisu aktualizacji" ) );
			}
			else
				endWork( tr( "Program jest aktualny" ) );
		}
		else
			endWork( tr( "Błąd sprawdzania aktualizacji" ) );
	}
	else
		endWork( tr( "Błąd sprawdzania aktualizacji" ) );
	infoFile.remove();
	reply->deleteLater();
}
void Updater::headerChanged()
{
	QNetworkReply *reply = ( QNetworkReply * )sender();
	QUrl newUrl = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
	if ( !newUrl.isEmpty() && reply->url() != newUrl )
	{
		disconnect( reply, SIGNAL( finished() ), this, SLOT( downloadFinished() ) );
		reply->abort();
		reply->deleteLater();
		getFile( newUrl );
	}
}
void Updater::writeToFile()
{
	QNetworkReply *reply = ( QNetworkReply * )sender();
	QByteArray arr = reply->readAll();
	bool err = false;
	if ( firstChunk )
	{
#if defined Q_OS_WIN
		err = arr.left( 2 ) != "MZ";
#elif defined Q_OS_LINUX
		err = arr.left( 4 ) != "\x7F""ELF" && arr.left( 2 ) != "#!";
#endif
		firstChunk = false;
	}
	if ( err || updateFile.write( arr ) != arr.size() )
		reply->abort();
}
void Updater::downloadprogress( qint64 bytesReceived, qint64 bytesTotal )
{
	progressB->setMaximum( bytesTotal );
	progressB->setValue( bytesReceived );
}
void Updater::downloadFinished()
{
	QNetworkReply *reply = ( QNetworkReply * )sender();
	if ( updateFile.size() && !reply->error() )
	{
#ifndef Q_OS_WIN
		updateFile.setPermissions( updateFile.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther );
#endif
		updateFile.close();
		QMPlay2Core.getSettings().set( "UpdateFile", updateFile.fileName() );
		infoL->setText( tr( "Aktualizacja została pobrana" ) );
		installB->show();
		busy = false;
	}
	else
	{
		updateFile.remove();
		endWork( tr( "Błąd pobierania aktualizacji" ) );
	}
	reply->deleteLater();
}

void Updater::applyUpdate()
{
	QMPlay2GUI.runUpdate( updateFile.fileName() );
	close();
	QMPlay2GUI.mainW->close();
}

void Updater::getFile( const QUrl &url )
{
	QNetworkRequest request( url );
	request.setRawHeader( "User-Agent", "QMPlay2" );
	QNetworkReply *reply = net.get( request );
	connect( reply, SIGNAL( metaDataChanged() ), this, SLOT( headerChanged() ) );
	connect( reply, SIGNAL( readyRead() ), this, SLOT( writeToFile() ) );
	connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( downloadprogress( qint64, qint64 ) ) );
	connect( reply, SIGNAL( finished() ), this, SLOT( downloadFinished() ) );
	reply->ignoreSslErrors();
	firstChunk = true;
}
void Updater::endWork( const QString &str )
{
	infoL->setText( str );
	progressB->hide();
	downloadUpdateB->show();
	busy = false;
}
