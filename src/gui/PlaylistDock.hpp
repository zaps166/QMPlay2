#include <DockWidget.hpp>

class QTreeWidgetItem;
class PlaylistWidget;
class QGridLayout;
class LineEdit;
class QLabel;

class PlaylistDock : public DockWidget
{
	Q_OBJECT
public:
	PlaylistDock();

	void stopThreads();

	QString getUrl(QTreeWidgetItem *tWI = NULL) const;
	QString getCurrentItemName() const;

	void load(const QString &);
	bool save(const QString &, bool saveCurrentGroup = false);

	void add(const QStringList &);
	void addAndPlay(const QStringList &);
	void add(const QString &);
	void addAndPlay(const QString &);

	inline QWidget *findEdit() const
	{
		return (QWidget *)findE;
	}
private:
	void expandTree(QTreeWidgetItem *);

	QWidget mainW;
	QGridLayout *layout;
	PlaylistWidget *list;
	QLabel *statusL;
	LineEdit *findE;

	enum RepeatMode {Normal, RepeatEntry, RepeatGroup, RepeatList, Random, RandomGroup};
	RepeatMode repeatMode;

	bool playAfterAdd;
	QTreeWidgetItem *lastPlaying;
	QList<QTreeWidgetItem *> randomPlayedItems;
private slots:
	void itemDoubleClicked(QTreeWidgetItem *);
	void addAndPlay(QTreeWidgetItem *);
public slots:
	void stopLoading();
	void next(bool playingError = false);
	void prev();
	void start();
	void clearCurrentPlaying();
	void setCurrentPlaying();
	void newGroup();
	void delEntries();
	void delNonGroupEntries();
	void clear();
	void copy();
	void paste();
	void renameGroup();
	void entryProperties();
	void timeSort1();
	void timeSort2();
	void titleSort1();
	void titleSort2();
	void collapseAll();
	void expandAll();
	void goToPlayback();
	void queue();
	void findItems(const QString &);
	void findNext();
	void visibleItemsCount(int);
	void syncCurrentFolder();
	void repeat();
	void updateCurrentEntry(const QString &, int);
signals:
	void play(const QString &);
	void stop();
};
