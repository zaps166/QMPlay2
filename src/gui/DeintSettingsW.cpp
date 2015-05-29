#include <DeintSettingsW.hpp>

#include <Settings.hpp>
#include <Module.hpp>
#include <Main.hpp>

#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

void DeintSettingsW::init()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.init( "Deinterlace/ON", true );
	QMPSettings.init( "Deinterlace/Auto", true );
	QMPSettings.init( "Deinterlace/Doubler", true );
	QMPSettings.init( "Deinterlace/AutoParity", true );
	QMPSettings.init( "Deinterlace/SoftwareMethod", "Bob" );
	QMPSettings.init( "Deinterlace/TFF", false );
}

DeintSettingsW::DeintSettingsW()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();

	setCheckable( true );
	setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );

	setChecked( QMPSettings.getBool( "Deinterlace/ON" ) );
	setTitle( tr( "Usuwaj przeplot" ) );

	autoDeintB = new QCheckBox( tr( "Automatycznie wykrywaj film z przeplotem" ) );
	autoDeintB->setChecked( QMPSettings.getBool( "Deinterlace/Auto" ) );

	doublerB = new QCheckBox( tr( "Podwajaj liczbę klatek na sekundę" ) );
	doublerB->setChecked( QMPSettings.getBool( "Deinterlace/Doubler" ) );
	connect( doublerB, SIGNAL( clicked( bool ) ), this, SLOT( softwareMethods( bool ) ) );

	autoParityB = new QCheckBox( tr( "Automatycznie wykryj parzystość" ) );
	autoParityB->setChecked( QMPSettings.getBool( "Deinterlace/AutoParity" ) );

	QLabel *methodL = new QLabel( tr( "Metoda usuwania przeplotu (dekodowanie programowe)" ) + ": " );

	softwareMethodsCB = new QComboBox;
	softwareMethodsCB->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );

	QLabel *parityL = new QLabel( tr( "Parzystość (jeżeli nie wykryto automatycznie)" ) + ": " );

	parityCB = new QComboBox;
	parityCB->addItems( QStringList() << "Bottom field first" << "Top field first" );
	parityCB->setCurrentIndex( QMPSettings.getBool( "Deinterlace/TFF" ) );

	softwareMethods( doublerB->isChecked() );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( autoDeintB, 0, 0, 1, 3 );
	layout->addWidget( doublerB, 1, 0, 1, 3 );
	layout->addWidget( autoParityB, 2, 0, 1, 3 );
	layout->addWidget( methodL, 3, 0, 1, 1 );
	layout->addWidget( softwareMethodsCB, 3, 1, 1, 2 );
	layout->addWidget( parityL, 4, 0, 1, 1 );
	layout->addWidget( parityCB, 4, 1, 1, 2 );
	layout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ), layout->rowCount(), 0 );
}

void DeintSettingsW::writeSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.set( "Deinterlace/ON", isChecked() );
	QMPSettings.set( "Deinterlace/Auto", autoDeintB->isChecked() );
	QMPSettings.set( "Deinterlace/Doubler", doublerB->isChecked() );
	QMPSettings.set( "Deinterlace/AutoParity", autoParityB->isChecked() );
	QMPSettings.set( "Deinterlace/SoftwareMethod", softwareMethodsCB->currentText() );
	QMPSettings.set( "Deinterlace/TFF", ( bool )parityCB->currentIndex() );
}

void DeintSettingsW::softwareMethods( bool doubler )
{
	softwareMethodsCB->clear();
	foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
		foreach ( const Module::Info &mod, module->getModulesInfo() )
			if ( ( mod.type & 0xF ) == Module::VIDEOFILTER && ( mod.type & Module::DEINTERLACE ) && ( doubler == ( bool )( mod.type & Module::DOUBLER ) ) )
				softwareMethodsCB->addItem( mod.name );
	const int idx = softwareMethodsCB->findText( QMPlay2Core.getSettings().getString( "Deinterlace/SoftwareMethod" ) );
	softwareMethodsCB->setCurrentIndex( idx < 0 ? 0 : idx );
}
