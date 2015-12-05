#include <EntryProperties.hpp>

#ifdef QMPlay2_TagEditor
	#include <TagEditor.hpp>
#endif
#include <PlaylistWidget.hpp>
#include <AddressBox.hpp>
#include <Functions.hpp>
#include <Main.hpp>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QToolButton>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>

EntryProperties::EntryProperties( QWidget *p, QTreeWidgetItem *_tWI, bool &sync, bool &accepted ) :
	QDialog( p ), sync( sync )
{
	sync = false;
	tWI = _tWI;
	if ( !tWI )
		return;

	catalogCB = NULL;
	browseB = NULL;
	dirPthE = NULL;
	addrB = NULL;
#ifdef QMPlay2_TagEditor
	tagEditor = NULL;
#endif
	fileSizeL = NULL;

	setWindowTitle( tr( "Właściwości" ) );

	QGridLayout layout( this );

	nameE = new QLineEdit;
	nameE->setText( tWI->text( 0 ) );

	QString url = tWI->data( 0, Qt::UserRole ).toString();
	if ( url.startsWith( "file://" ) )
		url.remove( 0, 7 );

	if ( PlaylistWidget::isGroup( tWI ) )
	{
		dirPthE = new QLineEdit;
		dirPthE->setText( url );
		origDirPth = url;

		nameE->selectAll();

		catalogCB = new QCheckBox;
		catalogCB->setText( tr( "Synchronizuj z katalogiem" ) );
		catalogCB->setChecked( !dirPthE->text().isEmpty() );
		connect( catalogCB, SIGNAL( stateChanged( int ) ), this, SLOT( setDirPthEEnabled( int ) ) );

		browseB = new QToolButton;
		browseB->setIcon( QMPlay2Core.getIconFromTheme( "folder-open" ) );
		connect( browseB, SIGNAL( clicked() ), this, SLOT( browse() ) );

		setDirPthEEnabled( catalogCB->isChecked() );
	}
	else
	{
		addrB = new AddressBox( Qt::Horizontal, url );

#ifdef QMPlay2_TagEditor
		tagEditor = new TagEditor;
		connect( addrB, SIGNAL( directAddressChanged() ), this, SLOT( directAddressChanged() ) );
		directAddressChanged();
#endif

		QFileInfo fi( addrB->cleanUrl() );
		if ( fi.isFile() )
			fileSizeL = new QLabel( tr( "Rozmiar pliku" ) + ": " + Functions::sizeString( fi.size() ) );

		nameE->setReadOnly( true );
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox;
	buttonBox->setStandardButtons( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
	connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );

	int row = 0;
	layout.addWidget( nameE, row++, 0, 1, browseB ? 2 : 1 );
	if ( catalogCB )
		layout.addWidget( catalogCB, row++, 0, 1, 1 );
	if ( addrB )
		layout.addWidget( addrB, row, 0, 1, 1 );
#ifdef QMPlay2_TagEditor
	if ( tagEditor )
		layout.addWidget( tagEditor, ++row, 0, 1, 1 );
#endif
	if ( fileSizeL )
		layout.addWidget( fileSizeL, ++row, 0, 1, 1 );
	if ( dirPthE )
		layout.addWidget( dirPthE, row, 0, 1, 1 );
	if ( browseB )
		layout.addWidget( browseB, row, 1, 1, 1 );
#ifdef QMPlay2_TagEditor
	if ( !tagEditor )
#endif
		layout.addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ), ++row, 0, 1, 2 ); //vSpacer
	layout.addWidget( buttonBox, ++row, 0, 1, browseB ? 2 : 1 );
	layout.setMargin( 3 );

#ifdef QMPlay2_TagEditor
	resize( 625, tagEditor ? 470 : 0 );
#else
	resize( 625, 0 );
#endif

	accepted = exec() == QDialog::Accepted;
}

void EntryProperties::setDirPthEEnabled( int e )
{
	dirPthE->setEnabled( e );
	browseB->setEnabled( e );
}
#ifdef QMPlay2_TagEditor
void EntryProperties::directAddressChanged()
{
	bool e = addrB->currentPrefixType() == AddressBox::DIRECT;
	if ( e )
		e = tagEditor->open( addrB->url() );
	if ( !e )
		tagEditor->clear();
	tagEditor->setEnabled( e );
}
#endif
void EntryProperties::browse()
{
	QString s = QFileDialog::getExistingDirectory( this, tr( "Wybierz katalog" ), dirPthE->text() );
	if ( !s.isEmpty() )
	{
		if ( nameE->text().isEmpty() )
			nameE->setText( Functions::fileName( s ) );
		dirPthE->setText( s );
	}
}
void EntryProperties::accept()
{
	if ( catalogCB )
	{
		const QString newDirPth = dirPthE->text();
		const QFileInfo dirPthInfo = ( newDirPth );
		const bool isFile = dirPthInfo.isFile();
		const bool isDir  = dirPthInfo.isDir();
		if ( catalogCB->isChecked() && ( ( !isDir && !isFile ) || ( isFile && origDirPth != newDirPth ) ) )
		{
			QMessageBox::information( this, tr( "Zła ścieżka" ), tr( "Podaj ścieżkę do istniejącego katalogu" ) );
			return;
		}
		else if ( catalogCB->isChecked() && ( isDir || isFile ) )
		{
			if ( nameE->text().isEmpty() )
				nameE->setText( Functions::fileName( dirPthE->text() ) );
			tWI->setData( 0, Qt::UserRole, "file://" + dirPthE->text() );
			tWI->setIcon( 0, *QMPlay2GUI.folderIcon );
			sync = isDir;
		}
		else if ( !catalogCB->isChecked() )
		{
			tWI->setData( 0, Qt::UserRole, QString() );
			tWI->setIcon( 0, *QMPlay2GUI.groupIcon );
		}
		if ( !nameE->isReadOnly() )
			tWI->setText( 0, nameE->text() );
	}
	else
	{
		QString url = addrB->url();
		QString scheme = Functions::getUrlScheme( url );
		if ( addrB->currentPrefixType() == AddressBox::DIRECT && ( scheme.isEmpty() || scheme.length() == 1 /*Drive letter in Windows*/ ) )
			url.prepend( "file://" );
		tWI->setData( 0, Qt::UserRole, url );
#ifdef QMPlay2_TagEditor
		if ( tagEditor->isEnabled() && !tagEditor->save() )
			QMessageBox::critical( this, tagEditor->title(), tr( "Błąd podczas zapisu tagów, sprawdź czy masz uprawnienia do modyfikacji pliku!" ) );
#endif
	}
	QDialog::accept();
}
void EntryProperties::reject()
{
	if ( PlaylistWidget::isGroup( tWI ) && tWI->text( 0 ).isEmpty() )
		delete tWI;
	QDialog::reject();
}
