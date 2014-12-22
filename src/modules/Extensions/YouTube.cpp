#include <YouTube.hpp>

#include <Functions.hpp>
#include <LineEdit.hpp>
#include <Reader.hpp>

#include <QStringListModel>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextDocument>
#include <QProgressBar>
#include <QApplication>
#include <QHeaderView>
#include <QGridLayout>
#include <QMouseEvent>
#include <QToolButton>
#include <QCompleter>
#include <QClipboard>
#include <QMimeData>
#include <QSpinBox>
#include <QProcess>
#include <QAction>
#include <QLabel>
#include <QDebug>
#include <QDrag>
#include <QFile>

static QMap< int, QString > itag_arr;

static inline QUrl getGdataUrl( const QString &title, const int resultsPerPage, const int start_page )
{
	return QString( "http://gdata.youtube.com/feeds/api/videos?max-results=%1&start-index=%2&q=%3" ).arg( resultsPerPage ).arg( ( start_page - 1 ) * resultsPerPage + 1 ).arg( title );
}
static inline QUrl getAutocompleteUrl( const QString &text )
{
	return QString( "http://suggestqueries.google.com/complete/search?client=firefox&ds=yt&q=%1" ).arg( text );
}

static inline QString getFileExtension( const QString &ItagName )
{
	if ( ItagName.contains( "WebM" ) )
		return ".webm";
	else if ( ItagName.contains( "MP4" ) )
		return ".mp4";
	return ".unknown";
}

static inline QString getQMPlay2Url( const QTreeWidgetItem *tWI )
{
	if ( tWI->parent() )
		return "YouTube://{" + tWI->parent()->data( 0, Qt::UserRole ).toString() + "}" + tWI->data( 0, Qt::UserRole + 1 ).toString();
	return "YouTube://{" + tWI->data( 0, Qt::UserRole ).toString() + "}";
}

static inline QString unescape( const QString &str )
{
	return QByteArray::fromPercentEncoding( str.toLatin1() );
}

static QString fromU( QString s )
{
	int uIdx = 0;
	for ( ;; )
	{
		uIdx = s.indexOf( "\\u", uIdx );
		if ( uIdx == -1 )
			break;
		bool ok;
		const QChar chr = s.mid( uIdx + 2, 4 ).toUShort( &ok, 16 );
		if ( ok )
		{
			s.replace( uIdx, 6, chr );
			++uIdx;
		}
		else
			uIdx += 6;
	}
	return s;
}

/**/

class YouTubeDL : public BasicIO
{
public:
	inline YouTubeDL( const QString &youtubedl ) :
		youtubedl( youtubedl )
	{}

	void addr( const QString &url, const QString &param, QString *stream_url, QString *name, QString *extension )
	{
		if ( stream_url || name )
		{
			QStringList paramList = QStringList() << "-e";
			if ( !param.isEmpty() )
				paramList << "-f" << param;
			const QStringList ytdl_stdout = exec( url, paramList );
			if ( !ytdl_stdout.isEmpty() )
			{
				QString tmp_url = ytdl_stdout.size() == 1 ? ytdl_stdout[ 0 ] : ytdl_stdout[ 1 ];
				if ( stream_url )
					*stream_url = tmp_url;
				if ( name && ytdl_stdout.size() > 1 )
					*name = ytdl_stdout[ 0 ];
				if ( extension )
				{
					if ( tmp_url.contains( ".mp4" ) )
						*extension = ".mp4";
					else if ( tmp_url.contains( ".webm" ) )
						*extension = ".webm";
					else if ( tmp_url.contains( ".mkv" ) )
						*extension = ".mkv";
					else if ( tmp_url.contains( ".3gp" ) )
						*extension = ".3gp";
					else if ( tmp_url.contains( ".mpg" ) )
						*extension = ".mpg";
					else if ( tmp_url.contains( ".mpeg" ) )
						*extension = ".mpeg";
				}
			}
		}
	}

	QStringList exec( const QString &url, const QStringList &args )
	{
#ifndef Q_OS_WIN
		QFile youtube_dl_file( youtubedl );
		if ( youtube_dl_file.exists() )
		{
			QFile::Permissions exeFlags = QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
			if ( ( youtube_dl_file.permissions() & exeFlags ) != exeFlags )
				youtube_dl_file.setPermissions( youtube_dl_file.permissions() | exeFlags );
		}
#endif
		youtubedl_process.start( youtubedl, QStringList() << url << "-g" << args );
		if ( youtubedl_process.waitForFinished() )
		{
			if ( youtubedl_process.exitCode() )
			{
				QString error = youtubedl_process.readAllStandardError();
				int idx = error.indexOf( "ERROR:" );
				if ( idx > -1 )
					error.remove( 0, idx );
				emit QMPlay2Core.sendMessage( error, YouTubeName + QString( " (%1)" ).arg( Functions::fileName( youtubedl ) ), 3, 0 );
				return QStringList();
			}
			return QString( youtubedl_process.readAllStandardOutput() ).split( '\n', QString::SkipEmptyParts );
		}
		else if ( youtubedl_process.error() == QProcess::FailedToStart )
			emit QMPlay2Core.sendMessage( /*tr*/( "Brakuje zewnętrznego programu - 'youtube-dl'. Pobierz go, a następnie ustaw do niego ścieżkę w opcjach modułu YouTube." ), YouTubeName, 2, 0 );
		return QStringList();
	}
private:
	void abort()
	{
		youtubedl_process.kill();
	}

	QProcess youtubedl_process;
	QString youtubedl;

};

/**/

ResultsYoutube::ResultsYoutube()
{
	setAnimated( true );
	setIndentation( 12 );
	setExpandsOnDoubleClick( false );
	setEditTriggers( QAbstractItemView::NoEditTriggers );

	headerItem()->setText( 0, tr( "Tytuł" ) );
	headerItem()->setText( 1, tr( "Długość" ) );
	headerItem()->setText( 2, tr( "Użytkownik" ) );
	headerItem()->setText( 3, tr( "Ilość wyświetleń" ) );

	header()->setStretchLastSection( false );
#if QT_VERSION < 0x050000
	header()->setResizeMode( 0, QHeaderView::Stretch );
	header()->setResizeMode( 1, QHeaderView::ResizeToContents );
#else
	header()->setSectionResizeMode( 0, QHeaderView::Stretch );
	header()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
#endif

	connect( this, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ), this, SLOT( playEntry( QTreeWidgetItem * ) ) );
	connect( this, SIGNAL( customContextMenuRequested( const QPoint & ) ), this, SLOT( contextMenu( const QPoint & ) ) );
	setContextMenuPolicy( Qt::CustomContextMenu );
}

QTreeWidgetItem *ResultsYoutube::getDefaultQuality( const QTreeWidgetItem *tWI )
{
	if ( !tWI->childCount() )
		return NULL;
	foreach ( int itag, itags )
		for ( int i = 0 ; i < tWI->childCount() ; ++i )
			if ( tWI->child( i )->data( 0, Qt::UserRole + 2 ).toInt() == itag )
				return tWI->child( i );
	return tWI->child( 0 );
}

void ResultsYoutube::mouseMoveEvent( QMouseEvent *e )
{
	QTreeWidgetItem *tWI = currentItem();
	if ( tWI )
	{
		QString url;
		if ( e->buttons() & Qt::LeftButton )
			url = getQMPlay2Url( tWI );
		else if ( e->buttons() & Qt::MiddleButton ) //Link do strumienia
		{
			QTreeWidgetItem *tWI2 = tWI->parent() ? tWI : getDefaultQuality( tWI );
			if ( tWI2 )
				url = tWI2->data( 0, Qt::UserRole ).toString();
		}
		if ( !url.isEmpty() )
		{
			QMimeData *mimeData = new QMimeData;
			if ( e->buttons() & Qt::LeftButton )
				mimeData->setText( url );
			else if ( e->buttons() & Qt::MiddleButton )
				mimeData->setUrls( QList< QUrl >() << url );

			if ( tWI->parent() )
				tWI = tWI->parent();

			QDrag *drag = new QDrag( this );
			drag->setMimeData( mimeData );
			drag->setPixmap( tWI->data( 0, Qt::DecorationRole ).value< QPixmap >() );
			drag->exec( Qt::CopyAction | Qt::MoveAction | Qt::LinkAction );
			return;
		}
	}
	QTreeWidget::mouseMoveEvent( e );
}

void ResultsYoutube::enqueue()
{
	QTreeWidgetItem *tWI = currentItem();
	if ( tWI )
		emit QMPlay2Core.processParam( "enqueue", getQMPlay2Url( tWI ) );
}
void ResultsYoutube::playCurrentEntry()
{
	playEntry( currentItem() );
}
void ResultsYoutube::openPage()
{
	QTreeWidgetItem *tWI = currentItem();
	if ( tWI )
	{
		if ( tWI->parent() )
			tWI = tWI->parent();
		QMPlay2Core.run( tWI->data( 0, Qt::UserRole ).toString() );
	}
}
void ResultsYoutube::copyPageURL()
{
	QTreeWidgetItem *tWI = currentItem();
	if ( tWI )
	{
		if ( tWI->parent() )
			tWI = tWI->parent();
		QMimeData *mimeData = new QMimeData;
		mimeData->setText( tWI->data( 0, Qt::UserRole ).toString() );
		qApp->clipboard()->setMimeData( mimeData );
	}
}
void ResultsYoutube::copyStreamURL()
{
	const QString streamUrl = sender()->property( "StreamUrl" ).toString();
	if ( !streamUrl.isEmpty() )
	{
		QMimeData *mimeData = new QMimeData;
		mimeData->setText( streamUrl );
		qApp->clipboard()->setMimeData( mimeData );
	}
}

void ResultsYoutube::playEntry( QTreeWidgetItem *tWI )
{
	if ( tWI )
		emit QMPlay2Core.processParam( "open", getQMPlay2Url( tWI ) );
}

void ResultsYoutube::contextMenu( const QPoint &point )
{
	menu.clear();
	QTreeWidgetItem *tWI = currentItem();
	if ( tWI )
	{
		const bool isOK = !tWI->isDisabled();
		if ( isOK )
		{
			menu.addAction( tr( "Kolejkuj" ), this, SLOT( enqueue() ) );
			menu.addAction( tr( "Odtwórz" ), this, SLOT( playCurrentEntry() ) );
			menu.addSeparator();
		}
		menu.addAction( tr( "Otwórz stronę w przeglądarce" ), this, SLOT( openPage() ) );
		menu.addAction( tr( "Kopiuj adres strony" ), this, SLOT( copyPageURL() ) );
		menu.addSeparator();
		if ( isOK )
		{
			QVariant streamUrl;
			QTreeWidgetItem *tWI_2 = tWI;
			if ( !tWI_2->parent() )
				tWI_2 = getDefaultQuality( tWI_2 );
			if ( tWI_2 )
				streamUrl = tWI_2->data( 0, Qt::UserRole );

			if ( !streamUrl.isNull() )
			{
				menu.addAction( tr( "Kopiuj adres strumienia" ), this, SLOT( copyStreamURL() ) )->setProperty( "StreamUrl", streamUrl );
				menu.addSeparator();
			}

			const QString name = tWI->parent() ? tWI->parent()->text( 0 ) : tWI->text( 0 );
			foreach ( QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList() )
				if ( !dynamic_cast< YouTube * >( QMPlay2Ext ) )
				{
					QString addressPrefixName, url, param;
					if ( Functions::splitPrefixAndUrlIfHasPluginPrefix( getQMPlay2Url( tWI ), &addressPrefixName, &url, &param ) )
						if ( QAction *act = QMPlay2Ext->getAction( name, -2, url, addressPrefixName, param ) )
						{
							act->setParent( &menu );
							menu.addAction( act );
						}
				}
		}
		menu.popup( viewport()->mapToGlobal( point ) );
	}
}

/**/

PageSwitcher::PageSwitcher( QWidget *youTubeW )
{
	prevB = new QToolButton;
	connect( prevB, SIGNAL( clicked() ), youTubeW, SLOT( prev() ) );
	prevB->setAutoRaise( true );
	prevB->setArrowType( Qt::LeftArrow );

	currPageB = new QSpinBox;
	connect( currPageB, SIGNAL( editingFinished() ), youTubeW, SLOT( chPage() ) );
	currPageB->setMinimum( 1 );

	pagesL = new QLabel;

	nextB = new QToolButton;
	connect( nextB, SIGNAL( clicked() ), youTubeW, SLOT( next() ) );
	nextB->setAutoRaise( true );
	nextB->setArrowType( Qt::RightArrow );

	QHBoxLayout *hLayout = new QHBoxLayout( this );
	hLayout->setContentsMargins( 0, 0, 0, 0 );
	hLayout->addWidget( prevB );
	hLayout->addWidget( currPageB );
	hLayout->addWidget( pagesL );
	hLayout->addWidget( nextB );
}

/**/

YouTubeW::YouTubeW( QWidget *parent ) :
	QWidget( parent ),
	imgSize( QSize( 56, 56 ) ),
	completer( new QCompleter( new QStringListModel, this ) ),
	resultsPerPage( 25 ), currPage( 1 ),
	autocompleteReply( NULL ), searchReply( NULL ),
	net( this )
{
	dw = new DockWidget;
	connect( dw, SIGNAL( visibilityChanged( bool ) ), this, SLOT( setEnabled( bool ) ) );
	dw->setWindowTitle( "YouTube" );
	dw->setObjectName( YouTubeName );
	dw->setWidget( this );

	completer->setCaseSensitivity( Qt::CaseInsensitive );

	searchE = new LineEdit;
	connect( searchE, SIGNAL( textEdited( const QString & ) ), this, SLOT( searchTextEdited( const QString & ) ) );
	connect( searchE, SIGNAL( clearButtonClicked() ), this, SLOT( search() ) );
	connect( searchE, SIGNAL( returnPressed() ), this, SLOT( search() ) );
	searchE->setCompleter( completer );

	searchB = new QToolButton;
	connect( searchB, SIGNAL( clicked() ), this, SLOT( search() ) );
	searchB->setIcon( QIcon( ":/browserengine" ) );
	searchB->setToolTip( tr( "Wyszukaj" ) );
	searchB->setAutoRaise( true );

	showSettingsB = new QToolButton;
	connect( showSettingsB, SIGNAL( clicked() ), this, SLOT( showSettings() ) );
	showSettingsB->setIcon( QMPlay2Core.getIconFromTheme( "configure" ) );
	showSettingsB->setToolTip( tr( "Ustawienia" ) );
	showSettingsB->setAutoRaise( true );

	resultsW = new ResultsYoutube;

	progressB = new QProgressBar;
	progressB->hide();

	pageSwitcher = new PageSwitcher( this );
	pageSwitcher->hide();

	connect( &net, SIGNAL( finished( QNetworkReply * ) ), this, SLOT( netFinished( QNetworkReply * ) ) );

	QGridLayout *layout = new QGridLayout;
	layout->addWidget( showSettingsB, 0, 0, 1, 1 );
	layout->addWidget( searchE, 0, 1, 1, 1 );
	layout->addWidget( searchB, 0, 2, 1, 1 );
	layout->addWidget( pageSwitcher, 0, 3, 1, 1 );
	layout->addWidget( resultsW, 1, 0, 1, 4 );
	layout->addWidget( progressB, 2, 0, 1, 4 );
	setLayout( layout );
}

void YouTubeW::showSettings()
{
	emit QMPlay2Core.showSettings( "Extensions" );
}

void YouTubeW::next()
{
	pageSwitcher->currPageB->setValue( pageSwitcher->currPageB->value() + 1 );
	chPage();
}
void YouTubeW::prev()
{
	pageSwitcher->currPageB->setValue( pageSwitcher->currPageB->value() - 1 );
	chPage();
}
void YouTubeW::chPage()
{
	if ( currPage != pageSwitcher->currPageB->value() )
	{
		currPage = pageSwitcher->currPageB->value();
		search();
	}
}

void YouTubeW::searchTextEdited( const QString &text )
{
	if ( autocompleteReply )
	{
		autocompleteReply->deleteLater();
		autocompleteReply = NULL;
	}
	if ( text.isEmpty() )
		( ( QStringListModel * )completer->model() )->setStringList( QStringList() );
	else
		autocompleteReply = net.get( QNetworkRequest( getAutocompleteUrl( text ) ) );
}
void YouTubeW::search()
{
	const QString title = searchE->text();
	deleteReplies();
	if ( autocompleteReply )
	{
		autocompleteReply->deleteLater();
		autocompleteReply = NULL;
	}
	if ( searchReply )
	{
		searchReply->deleteLater();
		searchReply = NULL;
	}
	resultsW->clear();
	if ( !title.isEmpty() )
	{
		if ( lastTitle != title || sender() == searchE || sender() == searchB )
			currPage = 1;
		searchReply = net.get( QNetworkRequest( getGdataUrl( title, resultsPerPage, currPage ) ) );
		progressB->setRange( 0, 0 );
		progressB->show();
	}
	else
	{
		pageSwitcher->hide();
		progressB->hide();
	}
	lastTitle = title;
}

void YouTubeW::netFinished( QNetworkReply *reply )
{
	const QUrl redirected = reply->property( "Redirected" ).toBool() ? QUrl() : reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
	QNetworkReply *redirectedReply = ( !reply->error() && redirected.isValid() ) ? net.get( QNetworkRequest( redirected ) ) : NULL;
	if ( redirectedReply )
	{
		redirectedReply->setProperty( "tWI", reply->property( "tWI" ) );
		redirectedReply->setProperty( "Redirected", true );
	}

	if ( reply->error() )
	{
		if ( reply == searchReply )
		{
			deleteReplies();
			resultsW->clear();
			lastTitle.clear();
			progressB->hide();
			pageSwitcher->hide();
			emit QMPlay2Core.sendMessage( tr( "Błąd połączenia" ), YouTubeName, 3 );
		}
	}
	else if ( !redirectedReply )
	{
		const QByteArray replyData = reply->readAll();
		if ( reply == autocompleteReply )
			setAutocomplete( replyData );
		else if ( reply == searchReply )
			setSearchResults( replyData );
		else if ( linkReplies.contains( reply ) )
			getYouTubeVideo( replyData, QString(), ( ( QTreeWidgetItem * )reply->property( "tWI" ).value< void * >() ) );
		else if ( imageReplies.contains( reply ) )
		{
			QPixmap p;
			if ( p.loadFromData( replyData ) )
				( ( QTreeWidgetItem * )reply->property( "tWI" ).value< void * >() )->setData( 0, Qt::DecorationRole, p.scaled( imgSize, Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
		}
	}

	if ( reply == autocompleteReply )
		autocompleteReply = redirectedReply;
	else if ( reply == searchReply )
		searchReply = redirectedReply;
	else if ( linkReplies.contains( reply ) )
	{
		linkReplies.removeOne( reply );
		if ( !redirectedReply )
			progressB->setValue( progressB->value() + 1 );
		else
			linkReplies += redirectedReply;
	}
	else if ( imageReplies.contains( reply ) )
	{
		imageReplies.removeOne( reply );
		if ( !redirectedReply )
			progressB->setValue( progressB->value() + 1 );
		else
			imageReplies += redirectedReply;
	}

	if ( progressB->isVisible() && linkReplies.isEmpty() && imageReplies.isEmpty() )
		progressB->hide();

	reply->deleteLater();
}

void YouTubeW::searchMenu()
{
	const QString name = sender()->property( "name" ).toString();
	if ( !name.isEmpty() )
	{
		if ( !dw->isVisible() )
			dw->show();
		dw->raise();
		sender()->setProperty( "name", QVariant() );
		searchE->setText( name );
		search();
	}
}

void YouTubeW::deleteReplies()
{
	while ( !linkReplies.isEmpty() )
		linkReplies.takeFirst()->deleteLater();
	while ( !imageReplies.isEmpty() )
		imageReplies.takeFirst()->deleteLater();
}

void YouTubeW::setAutocomplete( const QByteArray &data )
{
	QStringList suggestions = fromU( QString( data ).remove( '"' ).remove( '[' ).remove( ']' ) ).split( ',' );
	if ( suggestions.size() > 1 )
	{
		suggestions.removeFirst();
		( ( QStringListModel * )completer->model() )->setStringList( suggestions );
		if ( searchE->hasFocus() )
			completer->complete();
	}
}

QString YouTubeW::getDataFromXML( const QString &html, const QString &begin, const QString &end )
{
	int idx = html.indexOf( begin );
	if ( idx > -1 )
	{
		idx += begin.length();
		int end_idx = html.indexOf( end, idx );
		if ( end_idx > -1 )
			return html.mid( idx, end_idx - idx );
	}
	return QString();
}
QString YouTubeW::IntToStr2( const int i )
{
	if ( i < 10 )
		return "0" + QString::number( i );
	return QString::number( i );
}
void YouTubeW::setSearchResults( const QByteArray &data )
{
	int i;
	bool ok;
	QStringList splitted = QString( data ).split( "<entry>" );
	for ( i = 1 ; i < splitted.count() ; i++ )
	{
		QString title = getDataFromXML( splitted[ i ], "<title type='text'>", "</title>" );
		QString user = getDataFromXML( splitted[ i ], "<name>", "</name>" );
		QString duration = getDataFromXML( splitted[ i ], "<yt:duration seconds='", "'/>" );
		QString viewCount = getDataFromXML( splitted[ i ], "viewCount='", "'" );
		QString ytPagelink = getDataFromXML( splitted[ i ], "<media:player url='", "'/>" );
		QString image = getDataFromXML( splitted[ i ], "<media:thumbnail url='", "' " );

		const int t = duration.toInt( &ok );
		if ( ok )
		{
			int H, M, S;
			Functions::getHMS( t, H, M, S );
			duration.clear();
			if ( H > 0 )
				duration = IntToStr2( H ) + ":";
			duration += IntToStr2( M ) + ":" + IntToStr2( S );
		}

		int ampIdx = ytPagelink.indexOf( '&' );
		if ( ampIdx > -1 )
			ytPagelink.remove( ampIdx, ytPagelink.length() - ampIdx );

		const QString videoInfoLink = convertLink( ytPagelink );
		if ( !title.isEmpty() && !videoInfoLink.isEmpty() )
		{
			QTreeWidgetItem *tWI = new QTreeWidgetItem( resultsW );
			tWI->setDisabled( true );

			QTextDocument txtDoc;
			txtDoc.setHtml( title );

			tWI->setText( 0, txtDoc.toPlainText() );
			tWI->setToolTip( 0, txtDoc.toPlainText() );
			tWI->setText( 1, duration );
			tWI->setText( 2, user );
			tWI->setText( 3, viewCount );

			tWI->setData( 0, Qt::UserRole, ytPagelink );

			QNetworkReply *linkReply = net.get( QNetworkRequest( videoInfoLink ) );
			QNetworkReply *imageReply = net.get( QNetworkRequest( image ) );
			linkReply->setProperty( "tWI", qVariantFromValue( ( void * )tWI ) );
			imageReply->setProperty( "tWI", qVariantFromValue( ( void * )tWI ) );
			linkReplies += linkReply;
			imageReplies += imageReply;
		}
	}
	if ( i == 1 )
		resultsW->clear();
	else
	{
		int pageCount = getDataFromXML( splitted[ 0 ], "<openSearch:totalResults>", "</openSearch:totalResults>" ).toInt() / resultsPerPage;
		currPage = ( getDataFromXML( splitted[ 0 ], "<openSearch:startIndex>", "</openSearch:startIndex>" ).toInt() - 1 ) / resultsPerPage + 1;
		if ( pageCount * resultsPerPage > 999 ) //max 999 wyników
			pageCount = 999 / resultsPerPage;

		pageSwitcher->currPageB->setMaximum( pageCount );
		pageSwitcher->currPageB->setValue( currPage );
		pageSwitcher->pagesL->setText( "/ " + QString::number( pageCount ) );
		pageSwitcher->show();

		progressB->setMaximum( linkReplies.count() + imageReplies.count() );
		progressB->setValue( 0 );
	}
}

QString YouTubeW::convertLink( const QString &ytPageLink )
{
	int toAdd = 0;
	int start_idx = ytPageLink.indexOf( "watch?v=" );
	if ( start_idx == -1 )
	{
		start_idx = ytPageLink.indexOf( "video_id=" );
		toAdd = 1;
	}
	if ( start_idx > -1 )
	{
		start_idx += 8 + toAdd;
		int end_idx = ytPageLink.indexOf( "&", start_idx );
		if ( end_idx == -1 )
			end_idx = ytPageLink.length();
		return "http://www.youtube.com/watch?v=" + ytPageLink.mid( start_idx, end_idx - start_idx ) + "&gl=US&hl=en&has_verified=1";
	}
	return QString();
}
QStringList YouTubeW::getYouTubeVideo( const QString &data, const QString &PARAM, QTreeWidgetItem *tWI, const QString &url, IOController< YouTubeDL > *youtube_dl )
{
	QStringList ret;
	for ( int i = 0 ; i <= 1 ; ++i )
	{
		int streamsIdx = data.indexOf( QString( i ? "adaptive_fmts" : "url_encoded_fmt_stream_map" ) + "\": \"" ); //"adaptive_fmts" contains audio or video urls
		if ( streamsIdx > -1 )
		{
			streamsIdx += 30;
			int streamsEndIdx = data.indexOf( '"', streamsIdx );
			if ( streamsEndIdx > -1 )
			{
				foreach ( QString stream, data.mid( streamsIdx, streamsEndIdx - streamsIdx ).split( ',' ) )
				{
					int itag = -1;
					QString ITAG, URL, Signature;
					foreach ( QString streamParams, stream.split( "\\u0026" ) )
					{
						const QStringList paramL = streamParams.split( '=' );
						if ( paramL.count() == 2 )
						{
							if ( paramL[ 0 ] == "itag" )
							{
								ITAG = "itag=" + paramL[ 1 ];
								itag = paramL[ 1 ].toInt();
							}
							else if ( paramL[ 0 ] == "url" )
								URL = unescape( paramL[ 1 ] );
							else if ( paramL[ 0 ] == "sig" )
								Signature = paramL[ 1 ];
							else if ( paramL[ 0 ] == "s" )
								Signature = "ENCRYPTED";
						}
					}

					if ( !URL.isEmpty() && itag_arr.contains( itag ) && ( !Signature.isEmpty() || URL.contains( "signature" ) ) )
					{
						if ( !Signature.isEmpty() )
							URL += "&signature=" + Signature;
						if ( !tWI )
						{
							if ( ITAG == PARAM )
							{
								ret << URL << getFileExtension( itag_arr[ itag ] );
								++i; //ensures end of loop
								break;
							}
							else if ( PARAM.isEmpty() )
								ret << URL << getFileExtension( itag_arr[ itag ] ) << QString::number( itag );
						}
						else
						{
							QTreeWidgetItem *ch = new QTreeWidgetItem( tWI );
							ch->setText( 0, itag_arr[ itag ] ); //Tekst widoczny, informacje o jakości
							if ( !URL.contains( "ENCRYPTED" ) ) //youtube-dl działa za wolno, żeby go tu wykonać
								ch->setData( 0, Qt::UserRole + 0, URL ); //Adres do pliku
							ch->setData( 0, Qt::UserRole + 1, ITAG ); //Dodatkowy parametr
							ch->setData( 0, Qt::UserRole + 2, itag ); //Dodatkowy parametr (jako liczba)
						}
					}
				}
			}
		}
	}

	if ( PARAM.isEmpty() && ret.count() >= 3 ) //Wyszukiwanie domyślnej jakości
	{
		foreach ( int itag, resultsW->itags )
		{
			bool br = false;
			for ( int i = 2 ; i < ret.count() ; i += 3 )
				if ( ret[ i ].toInt() == itag )
				{
					if ( i != 2 )
					{
						ret[ 0 ] = ret[ i-2 ];
						ret[ 1 ] = ret[ i-1 ];
					}
					br = true;
					break;
				}
			if ( br )
				break;
		}
		ret.erase( ret.begin()+2, ret.end() );
	}

	if ( tWI ) //Włącza item
		tWI->setDisabled( false );
	else if ( ret.count() == 2 ) //Pobiera tytuł
	{
		int ytplayerIdx = data.indexOf( "ytplayer.config" );
		if ( ytplayerIdx > -1 )
		{
			int titleIdx = data.indexOf( "title\": \"", ytplayerIdx );
			if ( titleIdx > -1 )
			{
				int titleEndIdx = data.indexOf( '"', titleIdx += 9 );
				if ( titleEndIdx > -1 )
					ret << fromU( data.mid( titleIdx, titleEndIdx - titleIdx ) );
			}
		}
		if ( ret.count() == 2 )
			ret << "Can't find title";
	}

	if ( ret.count() == 3 && ret[ 0 ].contains( "ENCRYPTED" ) )
	{
		int itag_idx = ret[ 0 ].indexOf( "itag=" );
		if ( itag_idx > -1 && youtube_dl->assign( new YouTubeDL( youtubedl ) ) )
		{
			QStringList ytdl_stdout = ( *youtube_dl )->exec( url, QStringList() << "-f" << QString::number( atoi( ret[ 0 ].mid( itag_idx + 5 ).toLatin1() ) ) );
			if ( !ytdl_stdout.isEmpty() && !ytdl_stdout[ 0 ].isEmpty() )
				ret[ 0 ] = ytdl_stdout[ 0 ];
			else
				ret.clear();
			youtube_dl->clear();
		}
	}
	else if ( !tWI && ret.isEmpty() && youtube_dl->assign( new YouTubeDL( youtubedl ) ) ) //ex. 18+
	{
		QString stream_url, name, extension;
		( *youtube_dl )->addr( url, PARAM.right( PARAM.length() - 5 ), &stream_url, &name, &extension ); //extension doesn't work on youtube in this function
		if ( !stream_url.isEmpty() )
		{
			if ( name.isEmpty() )
				name = "Can't find title";
			ret << stream_url << extension << name;
		}
		youtube_dl->clear();
	}

	return ret;
}

/**/

YouTube::YouTube( Module &module )
{
	SetModule( module );
}

ItagNames YouTube::getItagNames( const QStringList &itagList )
{
	if ( itag_arr.isEmpty() )
	{
		/* Dane z Wikipedii */
		itag_arr[ 18 ] = "270p/360p MP4 (H.264 + AAC 96kbps)";
		itag_arr[ 22 ] = "720p MP4 (H.264 + AAC 192kbps)";
		itag_arr[ 37 ] = "1080p MP4 (H.264 + AAC 192kbps)";
		itag_arr[ 38 ] = "3072p MP4 (H.264 + AAC 192kbps)";
		itag_arr[ 43 ] = "360p WebM (VP8 + Vorbis 128kbps)";
		itag_arr[ 44 ] = "480p WebM (VP8 + Vorbis 128kbps)";
		itag_arr[ 45 ] = "720p WebM (VP8 + Vorbis 192kbps)";
		itag_arr[ 46 ] = "1080p WebM (VP8 + Vorbis 192kbps)";

		/* Moje dane */
		itag_arr[ 141 ] = "Audio MP4 (AAC 256kbps)";
		itag_arr[ 140 ] = "Audio MP4 (AAC 128kbps)";
		itag_arr[ 139 ] = "Audio MP4 (AAC 48kbps)";
	}
	ItagNames itagPair;
	for ( QMap< int, QString >::const_iterator it = itag_arr.begin(), it_end = itag_arr.end() ; it != it_end ; ++it )
	{
		itagPair.first += it.value();
		itagPair.second += it.key();
	}
	itagPair.first.swap( itagPair.first.count() - 1, itagPair.first.count() - 3 );
	itagPair.second.swap( itagPair.second.count() - 1, itagPair.second.count() - 3 );
	for ( int i = 0, j = 0 ; i < itagList.count() ; ++i )
	{
		const int idx = itagPair.second.indexOf( itagList[ i ].toInt() );
		if ( idx > -1 )
		{
			itagPair.first.swap( j, idx );
			itagPair.second.swap( j, idx );
			++j;
		}
	}
	return itagPair;
}

bool YouTube::set()
{
	w.resultsW->setColumnCount( sets().get( "YouTube/ShowAdditionalInfo" ).toBool() ? 4 : 2 );
	w.resultsW->itags = getItagNames( sets().get( "YouTube/ItagList" ).toStringList() ).second;
	w.youtubedl = sets().getString( "YouTube/youtubedl" );
	if ( w.youtubedl.isEmpty() )
		w.youtubedl = "youtube-dl";
	return true;
}

DockWidget *YouTube::getDockWidget()
{
	return w.dw;
}

QList< YouTube::AddressPrefix > YouTube::addressPrefixList( bool img )
{
	return QList< AddressPrefix >() << AddressPrefix( "YouTube", img ? QImage( ":/youtube" ) : QImage() ) << AddressPrefix( "youtube-dl", img ? QImage( ":/video" ) : QImage() );
}
void YouTube::convertAddress( const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl )
{
	if ( !stream_url && !name && !img )
		return;
	if ( prefix == "YouTube" )
	{
		if ( img )
			*img = QImage( ":/youtube" );
		if ( ioCtrl && ( stream_url || name ) )
		{
			IOController< Reader > &reader = ioCtrl->toRef< Reader >();
			if ( Reader::create( w.convertLink( url ), reader ) )
			{
				QByteArray replyData;
				while ( reader->readyRead() && !reader->atEnd() && replyData.size() < 0x200000 /*2MiB*/ )
				{
					QByteArray arr = reader->read( 4096 );
					if ( !arr.size() )
						break;
					replyData += arr;
				}
				reader.clear();

				const QStringList youTubeVideo = w.getYouTubeVideo( replyData, param, NULL, url, ioCtrl->toPtr< YouTubeDL >() );
				if ( youTubeVideo.count() == 3 )
				{
					if ( stream_url )
						*stream_url = youTubeVideo[ 0 ];
					if ( name )
						*name = youTubeVideo[ 2 ];
					if ( extension )
						*extension = youTubeVideo[ 1 ];
				}
			}
		}
	}
	else if ( prefix == "youtube-dl" )
	{
		if ( img )
			*img = QImage( ":/video" );
		if ( ioCtrl )
		{
			IOController< YouTubeDL > &youtube_dl = ioCtrl->toRef< YouTubeDL >();
			if ( ioCtrl->assign( new YouTubeDL( w.youtubedl ) ) )
			{
				youtube_dl->addr( url, param, stream_url, name, extension );
				ioCtrl->clear();
			}
		}
	}
}

QAction *YouTube::getAction( const QString &name, int, const QString &url, const QString &, const QString & )
{
	if ( name != url )
	{
		QAction *act = new QAction( YouTubeW::tr( "Wyszukaj w YouTube" ), NULL );
		act->connect( act, SIGNAL( triggered() ), &w, SLOT( searchMenu() ) );
		act->setIcon( QIcon( ":/youtube" ) );
		act->setProperty( "name", name );
		return act;
	}
	return NULL;
}
