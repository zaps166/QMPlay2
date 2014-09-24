#ifndef ENTRYPROPERTIES_HPP
#define ENTRYPROPERTIES_HPP

#include <QDialog>

class QTreeWidgetItem;
class QToolButton;
class AddressBox;
class QLineEdit;
class QCheckBox;
#ifdef QMPlay2_TagEditor
	class TagEditor;
#endif
class QLabel;

class EntryProperties : public QDialog
{
	Q_OBJECT
public:
	EntryProperties( QWidget *, QTreeWidgetItem *, bool &, bool & );
private:
	QTreeWidgetItem *tWI;
	QLineEdit *nameE, *dirPthE;
#ifdef QMPlay2_TagEditor
	TagEditor *tagEditor;
#endif
	QLabel *fileSizeL;
	QCheckBox *catalogCB;
	QToolButton *browseB;
	AddressBox *addrB;
	bool &sync;
private slots:
	void setDirPthEEnabled( int );
#ifdef QMPlay2_TagEditor
	void directAddressChanged();
#endif
	void browse();
	void accept();
	void reject();
};

#endif //ENTRYPROPERTIES_HPP
