#include <QMPlay2Extensions.hpp>

class QNetworkAccessManager;
class QListWidgetItem;
class QProgressBar;
class QListWidget;
class QLabel;

#include <QMenu>
#include <QIcon>

class Radio : public QWidget, public QMPlay2Extensions
{
	Q_OBJECT
public:
	Radio( Module & );
	~Radio();

	DockWidget *getDockWidget();
private slots:
	void visibilityChanged( bool );
	void popup( const QPoint & );
	void removeStation();

	void openLink();

	void downloadProgress( qint64, qint64 );
	void finished();
private:
	void addGroup( const QString & );
	void addStation( const QString &nazwa, const QString &URL, const QString &groupName, const QByteArray &img = QByteArray() );

	DockWidget *dw;

	bool once;
	QNetworkAccessManager *net;

	QListWidget *lW;
	QLabel *infoL;
	QProgressBar *progressB;

	QMenu popupMenu;

	QIcon qmp2Icon;
	const QString wlasneStacje;
	QListWidgetItem *nowaStacjaLWI;
};

#define RadioName "Radio Browser"
