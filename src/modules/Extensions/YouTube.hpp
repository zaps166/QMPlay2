#include <QMPlay2Extensions.hpp>

#include <QNetworkAccessManager>
#include <QTreeWidget>
#include <QThread>
#include <QMenu>
#include <QMap>

class QNetworkReply;
class QProgressBar;
class QToolButton;
class QCompleter;
class QSpinBox;
class LineEdit;
class QLabel;

/**/

class ResultsTree : public QTreeWidget
{
	Q_OBJECT
public:
	ResultsTree();

	QList< int > itags;
private:
	QTreeWidgetItem *getDefaultQuality( const QTreeWidgetItem *tWI );

	void mouseMoveEvent( QMouseEvent * );

	QMenu menu;
private slots:
	void enqueue();
	void playCurrentEntry();
	void openPage();
	void copyPageURL();
	void copyStreamURL();

	void playEntry( QTreeWidgetItem *tWI );

	void contextMenu( const QPoint &p );
};

/**/

class PageSwitcher : public QWidget
{
	Q_OBJECT
public:
	PageSwitcher( QWidget *youTubeW );

	QToolButton *prevB, *nextB;
	QSpinBox *currPageB;
	QLabel *pagesL;
};

/**/

class YouTubeW : public QWidget
{
	friend class YouTube;
	Q_OBJECT
public:
	YouTubeW( QWidget *parent = NULL );

	DockWidget *dw;
private slots:
	void showSettings();

	void next();
	void prev();
	void chPage();

	void searchTextEdited( const QString &text );
	void search();

	void netFinished( QNetworkReply *reply );

	void searchFromMenu();
private:
	void deleteReplies();

	void setAutocomplete( const QByteArray &data );

	QStringList youtube_dl_exec( const QString &url, const QStringList &args = QStringList() );

	QString getDataFromXML( const QString &html, const QString &begin, const QString &end );
	QString IntToStr2( const int i );
	void setSearchResults( const QByteArray &data );

	QString convertLink( const QString &ytPageLink );
	QStringList getYouTubeVideo( const QString &data, const QString &PARAM = QString(), QTreeWidgetItem *tWI = NULL, const QString &url = QString() ); //jeżeli ( tWI == NULL ) to zwraca {URL, file_extension, TITLE}

	LineEdit *searchE;
	QToolButton *showSettingsB, *searchB;
	ResultsTree *resultsW;
	QProgressBar *progressB;
	PageSwitcher *pageSwitcher;

	QString lastTitle;
	const QSize imgSize;
	QCompleter *completer;
	int resultsPerPage, currPage;

	QNetworkReply *autocompleteReply, *searchReply;
	QList< QNetworkReply * > linkReplies, imageReplies;
	QNetworkAccessManager net;

	QString youtubedl;
};

/**/

typedef QPair< QStringList, QList< int > > ItagNames;

class YouTube : public QMPlay2Extensions
{
public:
	YouTube( Module &module );

	static ItagNames getItagNames( const QStringList &itagList = QStringList() );

	bool set();

	DockWidget *getDockWidget();

	QList< AddressPrefix > addressPrefixList( bool );
	void convertAddress( const QString &, const QString &, const QString &, QString *, QString *, QImage *, bool, Reader *&, QMutex * );

	QAction *getAction( const QString &, int, const QString &, const QString &, const QString & );
private:
	YouTubeW w;
};

#define YouTubeName "YouTube Browser"
