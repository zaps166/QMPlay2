#ifndef PLAYLISTWIDGET_HPP
#define PLAYLISTWIDGET_HPP

#include <IOController.hpp>
#include <Functions.hpp>
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
	Q_OBJECT
public:
	UpdateEntryThr( PlaylistWidget &pLW );

	void updateEntry( QTreeWidgetItem *item, const QString &name = QString(), int length = -2 );

	void stop();
private:
	void run();

	IOController<> ioCtrl;
	PlaylistWidget &pLW;
	bool timeChanged;
	QMutex mutex;

	struct ItemToUpdate
	{
		QTreeWidgetItem *item;
		QString url;
		int oldLength;

		QString name;
		int length;
	};
	QQueue< ItemToUpdate > itemsToUpdate;

	struct ItemUpdated
	{
		QTreeWidgetItem *item;
		QString name;
		QImage img;

		bool updateLength;
		int length;
	};
private slots:
	void updateItemSlot( const ItemUpdated &iu );
	void finished();
signals:
	void updateItem( const ItemUpdated &iu );
};

class AddThr : public QThread
{
	friend class PlaylistWidget;
	Q_OBJECT
public:
	AddThr( PlaylistWidget &pLW );

	void setData( const QStringList &, QTreeWidgetItem *, bool, bool sync = false );
	void setData( const QString &, QTreeWidgetItem * );

	void stop();
private:
	void run();

	PlaylistWidget &pLW;
	QStringList urls;
	QTreeWidgetItem *par;
	bool loadList;
	IOController<> ioCtrl;
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

	QString getUrl( QTreeWidgetItem *tWI = NULL ) const;

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

	QTreeWidgetItem *newEntry( const Playlist::Entry &, QTreeWidgetItem *, const Functions::DemuxersInfo & );

	QList< QTreeWidgetItem * > getChildren( CHILDREN children = ALL_CHILDREN, const QTreeWidgetItem *parent = NULL ) const;

	bool canModify( bool all = true ) const;

	void enqueue();
	void refresh( REFRESH Refresh = REFRESH_ALL );

	void processItems( QList< QTreeWidgetItem * > *itemsToShow = NULL, bool hideGroups = false );

	QString currentPlayingUrl;
	QTreeWidgetItem *currentPlaying;
	QIcon currentPlayingItemIcon;

	QList< QTreeWidgetItem * > queue;

	AddThr addThr;
	UpdateEntryThr updateEntryThr;

	bool dontUpdateAfterAdd;

	static inline bool isGroup( QTreeWidgetItem *tWI )
	{
		return tWI ? ( bool )( tWI->flags() & Qt::ItemIsDropEnabled ) : false;
	}
private:
	void _add( const QStringList &, QTreeWidgetItem *parent, QTreeWidgetItem **firstItem, const Functions::DemuxersInfo &demuxersInfo, bool loadList = false );
	QTreeWidgetItem *insertPlaylistEntries( const Playlist::Entries &entries, QTreeWidgetItem *parent, const Functions::DemuxersInfo &demuxersInfo );

	void setEntryIcon( const QImage &, QTreeWidgetItem * );

	void mouseMoveEvent( QMouseEvent * );
	void dragEnterEvent( QDragEnterEvent * );
	void dragMoveEvent( QDragMoveEvent * );
	void dropEvent( QDropEvent * );
	void paintEvent( QPaintEvent * );
	void scrollContentsBy( int dx, int dy );

	QRect getArcRect( int size );

	bool internalDrag, selectAfterAdd, hasHiddenItems;
	QString currPthToSave;
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
