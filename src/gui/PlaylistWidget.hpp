#ifndef PLAYLISTWIDGET_HPP
#define PLAYLISTWIDGET_HPP

#include <Playlist.hpp>

#include <QTreeWidget>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QTimer>
#include <QUrl>

class QTreeWidgetItem;
class PlaylistWidget;
class Demuxer;

class UpdateEntryThr : public QThread
{
	friend class PlaylistWidget;
public:
	UpdateEntryThr( PlaylistWidget &pLW ) :
		convertAddressReader( NULL ),
		demuxer( NULL ),
		pLW( pLW )
	{}

	void updateEntry( QTreeWidgetItem *, const QString &name = QString(), int length = -2 );

	void stop();
private:
	void run();

	bool br;
	QMutex mutex, abortMutex;
	Reader *convertAddressReader;
	Demuxer *demuxer;

	struct ItemToUpdate
	{
		QTreeWidgetItem *tWI;
		QString name;
		int length;
	};
	QQueue< ItemToUpdate > itemsToUpdate;

	PlaylistWidget &pLW;
};

class AddThr : public QThread
{
	friend class PlaylistWidget;
	Q_OBJECT
public:
	AddThr( PlaylistWidget &pLW ) :
		pLW( pLW ),
		convertAddressReader( NULL ),
		demuxer( NULL )
	{
		connect( this, SIGNAL( finished() ), this, SLOT( finished() ) );
	}

	void setData( const QStringList &, QTreeWidgetItem *, bool, bool sync = false );
	void setData( const QString &, QTreeWidgetItem * );

	void stop();
private:
	void run();

	PlaylistWidget &pLW;
	QStringList urls;
	QTreeWidgetItem *par;
	bool loadList, br;
	Reader *convertAddressReader;
	Demuxer *demuxer;
	QMutex abortMutex;
	QTreeWidgetItem *firstI, *lastI;
private slots:
	void finished();
};

class PlaylistWidget : public QTreeWidget
{
	friend class AddThr;
	friend class UpdateEntryThr;
	Q_OBJECT
public:
	enum CHILDREN { ONLY_GROUPS = 0x10, ONLY_NON_GROUPS = 0x100, ALL_CHILDREN = ONLY_GROUPS | ONLY_NON_GROUPS };
	enum REFRESH  { REFRESH_QUEUE = 0x10, REFRESH_GROUPS_TIME = 0x100, REFRESH_CURRPLAYING = 0x1000, REFRESH_ALL = REFRESH_QUEUE | REFRESH_GROUPS_TIME | REFRESH_CURRPLAYING };

	PlaylistWidget();

	inline QString getUrl( QTreeWidgetItem *tWI = NULL ) const
	{
		if ( !tWI )
			tWI = currentItem();
		return tWI ? tWI->data( 0, Qt::UserRole ).toString() : QString();
	}

	void setItemsResizeToContents( bool );

	bool add( const QStringList &, QTreeWidgetItem *par, bool loadList = false );
	bool add( const QStringList &, bool atEndOfList = false );
	void sync( const QString &, QTreeWidgetItem * );

	void setCurrentPlaying( QTreeWidgetItem *tWI );

	void clear();
	void clearCurrentPlaying( bool url = true );

	void stopLoading();

	QList< QTreeWidgetItem * > topLevelNonGroupsItems() const;
	QList< QUrl > getUrls() const;

	QTreeWidgetItem *newGroup( const QString &name, const QString &url = QString(), QTreeWidgetItem *parent = NULL, bool insertChildAt0Idx = false );
	QTreeWidgetItem *newGroup();

	QTreeWidgetItem *newEntry( const Playlist::Entry &, QTreeWidgetItem * );

	QList< QTreeWidgetItem * > getChildren( CHILDREN children = ALL_CHILDREN, const QTreeWidgetItem *parent = NULL ) const;

	bool canModify( bool all = true ) const;

	void queue();
	void ref( REFRESH Refresh = REFRESH_ALL );

	void processItems( QList< QTreeWidgetItem * > *itemsToShow = NULL, bool hideGroups = false );

	QString currentPlayingUrl;
	QTreeWidgetItem *currentPlaying;
	QIcon currentPlayingItemIcon;

	QList< QTreeWidgetItem * > _queue;

	AddThr addThr;
	UpdateEntryThr updateEntryThr;

	bool dontUpdateAfterAdd;

	static inline bool isGroup( QTreeWidgetItem *tWI )
	{
		return tWI ? ( bool )( tWI->flags() & Qt::ItemIsDropEnabled ) : false;
	}
private:
	void _add( const QStringList &, QTreeWidgetItem *parent, QTreeWidgetItem **firstI, bool loadList = false );

	void setEntryIcon( QImage &, QTreeWidgetItem * );

	void mouseMoveEvent( QMouseEvent * );
	void dragEnterEvent( QDragEnterEvent * );
	void dragMoveEvent( QDragMoveEvent * );
	void dropEvent( QDropEvent * );
	void keyPressEvent( QKeyEvent * );
	void keyReleaseEvent( QKeyEvent * );
	void paintEvent( QPaintEvent * );
	void scrollContentsBy( int dx, int dy );

	QRect getArcRect( int size );

	bool Modifier, internalDrag, selectAfterAdd, _useThread, hasHiddenItems, saveCurrPth;
	struct AddData
	{
		QStringList urls;
		QTreeWidgetItem *par;
		bool loadList;
	};
	QQueue< AddData > enqueuedAddData;
	QTimer animationTimer, addTimer;
	bool repaintAll;
	int rotation;
private slots:
	void insertItemSlot( QTreeWidgetItem *, QTreeWidgetItem *, bool );
	void popupContextMenu( const QPoint & );
	void setItemIconSlot( QTreeWidgetItem *, const QImage & );
	void animationUpdate();
	void addTimerElapsed();
public slots:
	void modifyMenu();
signals:
	void returnItem( QTreeWidgetItem * );
	void setItemIcon( QTreeWidgetItem *, const QImage & );
	void visibleItemsCount( int );
	void insertItem( QTreeWidgetItem *, QTreeWidgetItem *, bool );
};

#endif
