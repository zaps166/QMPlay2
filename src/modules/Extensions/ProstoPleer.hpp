#include <QMPlay2Extensions.hpp>

#include <QNetworkAccessManager>
#include <QTreeWidget>
#include <QMenu>

/**/

class ResultsPleer : public QTreeWidget
{
	Q_OBJECT
public:
	ResultsPleer();
private:
	QMenu menu;
private slots:
	void enqueue();
	void playCurrentEntry();
	void openPage();
	void copyPageURL();

	void playEntry(QTreeWidgetItem *tWI);

	void contextMenu(const QPoint &p);
};

/**/

class QProgressBar;
class QToolButton;
class QCompleter;
class LineEdit;

class ProstoPleerW : public QWidget
{
	friend class ProstoPleer;
	Q_OBJECT
public:
	ProstoPleerW();
private slots:
	void next();

	void searchTextEdited(const QString &text);
	void search();

	void netFinished(QNetworkReply *reply);

	void searchMenu();
private:
	DockWidget *dw;

	LineEdit *searchE;
	QToolButton *searchB, *nextPageB;
	QProgressBar *progressB;
	ResultsPleer *resultsW;

	QCompleter *completer;
	QString lastName;
	int currPage;

	QNetworkReply *autocompleteReply, *searchReply;
	QNetworkAccessManager net;
};

/**/

class ProstoPleer : public QMPlay2Extensions
{
public:
	ProstoPleer(Module &module);

	bool set();

	DockWidget *getDockWidget();

	QList<AddressPrefix> addressPrefixList(bool);
	void convertAddress(const QString &prefix, const QString &url, const QString &param, QString *stream_url, QString *name, QImage *img, QString *extension, IOController<> *ioCtrl);

	QAction *getAction(const QString &, double, const QString &, const QString &, const QString &);
private:
	ProstoPleerW w;
};

#define ProstoPleerName "Prostopleer"
