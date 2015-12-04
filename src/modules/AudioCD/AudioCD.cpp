#include <AudioCD.hpp>
#include <AudioCDDemux.hpp>

#include <QDialogButtonBox>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialog>
#include <QAction>
#include <QLabel>

AudioCD::AudioCD() :
	Module( "AudioCD" ),
	cdioDestroyTimer( new CDIODestroyTimer )
{
	moduleImg = QImage( ":/AudioCD" );

	init( "AudioCD/CDDB", true );
	init( "AudioCD/CDTEXT", true );
}
AudioCD::~AudioCD()
{
	delete cdioDestroyTimer;
	libcddb_shutdown();
}

QList< AudioCD::Info > AudioCD::getModulesInfo( const bool ) const
{
	QList< Info > modulesInfo;
#ifdef Q_OS_WIN
	modulesInfo += Info( AudioCDName, DEMUXER, QStringList( "cda" ) );
#else
	modulesInfo += Info( AudioCDName, DEMUXER );
#endif
	return modulesInfo;
}
void *AudioCD::createInstance( const QString &name )
{
	if ( name == AudioCDName )
		return new AudioCDDemux( *this, *cdioDestroyTimer );
	return NULL;
}

QList< QAction * > AudioCD::getAddActions()
{
	QAction *actCD = new QAction( NULL );
	actCD->setIcon( QIcon( ":/AudioCD" ) );
	actCD->setText( tr( "Płyta AudioCD" ) );
	actCD->connect( actCD, SIGNAL( triggered() ), this, SLOT( add() ) );
	return QList< QAction * >() << actCD;
}

AudioCD::SettingsWidget *AudioCD::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

void AudioCD::add()
{
	QWidget *parent = qobject_cast< QWidget * >( sender()->parent() );
	QStringList drives = AudioCDDemux::getDevices();
	if ( !drives.isEmpty() )
	{
		QDialog chooseCD( parent );
		chooseCD.setWindowIcon( QIcon( ":/AudioCD" ) );
		chooseCD.setWindowTitle( tr( "Wybierz napęd" ) );
		QLabel drvL( tr( "Ścieżka" ) + ":" );
		drvL.setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred ) );
		QComboBox drvB;
		QLineEdit drvE;
		connect( &drvB, SIGNAL( currentIndexChanged( const QString & ) ), &drvE, SLOT( setText( const QString & ) ) );
		drvB.addItems( drives );
		QToolButton browseB;
		connect( &browseB, SIGNAL( clicked() ), this, SLOT( browseCDImage() ) );
		browseB.setIcon( QMPlay2Core.getIconFromTheme( "folder-open" ) );
		QDialogButtonBox bb( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
		connect( &bb, SIGNAL( accepted() ), &chooseCD, SLOT( accept() ) );
		connect( &bb, SIGNAL( rejected() ), &chooseCD, SLOT( reject() ) );
		QGridLayout layout( &chooseCD );
		layout.addWidget( &drvB, 0, 0, 1, 3 );
		layout.addWidget( &drvL, 1, 0, 1, 1 );
		layout.addWidget( &drvE, 1, 1, 1, 1 );
		layout.addWidget( &browseB, 1, 2, 1, 1 );
		layout.addWidget( &bb, 2, 0, 1, 3 );
		layout.setMargin( 2 );
		chooseCD.resize( 400, 0 );
		if ( chooseCD.exec() == QDialog::Accepted )
			emit QMPlay2Core.processParam( "open", AudioCDName "://" + drvE.text() );
	}
	else
		QMessageBox::information( parent, AudioCDName, tr( "Nie znaleziono napędów CD/DVD!" ) );
}
void AudioCD::browseCDImage()
{
	QWidget *parent = ( QWidget * )sender()->parent();
	QString path = QFileDialog::getOpenFileName( parent, tr( "Wybierz obraz AudioCD" ), QString(), tr( "Obsługiwane obrazy AudioCD" ) + " (*.cue *.nrg *.toc)" );
	if ( !path.isEmpty() )
	{
		QComboBox &drvB = *parent->findChild< QComboBox * >();
		drvB.addItem( path );
		drvB.setCurrentIndex( drvB.count() - 1 );
	}
}

QMPLAY2_EXPORT_PLUGIN( AudioCD )

/**/

#include <QRadioButton>
#include <QGroupBox>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	audioCDB = new QGroupBox( tr( "AudioCD" ) );

	useCDDB = new QCheckBox( tr( "Używaj CDDB, jeżeli CD-TEXT jest niedostępny" ) );
	useCDDB->setChecked( sets().getBool( "AudioCD/CDDB" ) );

	useCDTEXT = new QCheckBox( tr( "Używaj CD-TEXT" ) );
	useCDTEXT->setChecked( sets().getBool( "AudioCD/CDTEXT" ) );

	QVBoxLayout *audioCDBLayout = new QVBoxLayout( audioCDB );
	audioCDBLayout->addWidget( useCDDB );
	audioCDBLayout->addWidget( useCDTEXT );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( audioCDB );
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "AudioCD/CDDB", useCDDB->isChecked() );
	sets().set( "AudioCD/CDTEXT", useCDTEXT->isChecked() );
}
