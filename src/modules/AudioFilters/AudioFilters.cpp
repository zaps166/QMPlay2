#include <AudioFilters.hpp>
#include <Equalizer.hpp>
#include <EqualizerGUI.hpp>
#include <VoiceRemoval.hpp>
#include <PhaseReverse.hpp>
#include <Echo.hpp>

AudioFilters::AudioFilters() :
	Module( "AudioFilters" )
{
	moduleImg = QImage( ":/audiofilters" );

	init( "Equalizer", false );
	int nbits = getInt( "Equalizer/nbits" );
	if ( nbits < 8 || nbits > 16 )
		set( "Equalizer/nbits", 10 );
	int count = getInt( "Equalizer/count" );
	if ( count < 2 || count > 20 )
		set( "Equalizer/count", ( count = 8 ) );
	int minFreq = getInt( "Equalizer/minFreq" );
	if ( minFreq < 10 || minFreq > 300 )
		set( "Equalizer/minFreq", ( minFreq = 200 ) );
	int maxFreq = getInt( "Equalizer/maxFreq" );
	if ( maxFreq < 10000 || maxFreq > 96000 )
		set( "Equalizer/maxFreq", ( maxFreq = 18000 ) );
	init( "Equalizer/-1", 50 );
	for ( int i = 0 ; i < count ; ++i )
		init( "Equalizer/" + QString::number( i ), 50 );
	init( "VoiceRemoval", false );
	init( "PhaseReverse", false );
	init( "PhaseReverse/ReverseRight", false );
	init( "Echo", false );
	init( "Echo/Delay", 500 );
	init( "Echo/Volume", 50 );
	init( "Echo/Feedback", 50 );
	init( "Echo/Surround", false );
	if ( getBool( "Equalizer" ) )
	{
		bool disableEQ = true;
		for ( int i = -1 ; i < count ; ++i )
			disableEQ &= getInt( "Equalizer/" + QString::number( i ) ) == 50;
		if ( disableEQ )
			set( "Equalizer", false );
	}
}

QList< AudioFilters::Info > AudioFilters::getModulesInfo( const bool ) const
{
	QList< Info > modulesInfo;
	modulesInfo += Info( EqualizerName, AUDIOFILTER );
	modulesInfo += Info( EqualizerGUIName, QMPLAY2EXTENSION );
	modulesInfo += Info( VoiceRemovalName, AUDIOFILTER );
	modulesInfo += Info( PhaseReverseName, AUDIOFILTER );
	modulesInfo += Info( EchoName, AUDIOFILTER );
	return modulesInfo;
}
void *AudioFilters::createInstance( const QString &name )
{
	if ( name == EqualizerName )
		return new Equalizer( *this );
	else if ( name == EqualizerGUIName )
		return static_cast< QMPlay2Extensions * >( new EqualizerGUI( *this ) );
	else if ( name == VoiceRemovalName )
		return new VoiceRemoval( *this );
	else if ( name == PhaseReverseName )
		return new PhaseReverse( *this );
	else if ( name == EchoName )
		return new Echo( *this );
	return NULL;
}

AudioFilters::SettingsWidget *AudioFilters::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_PLUGIN( AudioFilters )

/**/

#include <Slider.hpp>

#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	voiceRemovalEB = new QCheckBox( tr( "Usuwanie głosu" ) );
	voiceRemovalEB->setChecked( sets().getBool( "VoiceRemoval" ) );
	connect( voiceRemovalEB, SIGNAL( clicked() ), this, SLOT( voiceRemovalToggle() ) );

	phaseReverseEB = new QCheckBox( tr( "Odwracanie fazy" ) );
	phaseReverseEB->setChecked( sets().getBool( "PhaseReverse" ) );
	connect( phaseReverseEB, SIGNAL( clicked() ), this, SLOT( phaseReverse() ) );

	phaseReverseRightB = new QCheckBox( tr( "Odwracaj fazę prawego kanału" ) );
	phaseReverseRightB->setChecked( sets().getBool( "PhaseReverse/ReverseRight" ) );
	connect( phaseReverseRightB, SIGNAL( clicked() ), this, SLOT( phaseReverse() ) );

	phaseReverseRightB->setEnabled( phaseReverseEB->isChecked() );

	echoB = new QGroupBox( tr( "Echo" ) );
	echoB->setCheckable( true );
	echoB->setChecked( sets().getBool( "Echo" ) );
	connect( echoB, SIGNAL( clicked() ), this, SLOT( echo() ) );

	QLabel *echoDelayL = new QLabel( tr( "Opóźnienie echa" ) + ": " );

	echoDelayB = new Slider;
	echoDelayB->setRange( 1, 1000 );
	echoDelayB->setValue( sets().getUInt( "Echo/Delay" ) );
	connect( echoDelayB, SIGNAL( valueChanged( int ) ), this, SLOT( echo() ) );

	QLabel *echoVolumeL = new QLabel( tr( "Głośność echa" ) + ": " );

	echoVolumeB = new Slider;
	echoVolumeB->setRange( 1, 100 );
	echoVolumeB->setValue( sets().getUInt( "Echo/Volume" ) );
	connect( echoVolumeB, SIGNAL( valueChanged( int ) ), this, SLOT( echo() ) );

	QLabel *echoFeedbackL = new QLabel( tr( "Powtarzanie echa" ) + ": " );

	echoFeedbackB = new Slider;
	echoFeedbackB->setRange( 1, 100 );
	echoFeedbackB->setValue( sets().getUInt( "Echo/Feedback" ) );
	connect( echoFeedbackB, SIGNAL( valueChanged( int ) ), this, SLOT( echo() ) );

	echoSurroundB = new QCheckBox( tr( "Przestrzenny dźwięk echa" ) );
	connect( echoSurroundB, SIGNAL( clicked() ), this, SLOT( echo() ) );

	QGridLayout *echoBLayout = new QGridLayout( echoB );
	echoBLayout->addWidget( echoDelayL, 0, 0, 1, 1 );
	echoBLayout->addWidget( echoDelayB, 0, 1, 1, 1 );
	echoBLayout->addWidget( echoVolumeL, 1, 0, 1, 1 );
	echoBLayout->addWidget( echoVolumeB, 1, 1, 1, 1 );
	echoBLayout->addWidget( echoFeedbackL, 2, 0, 1, 1 );
	echoBLayout->addWidget( echoFeedbackB, 2, 1, 1, 1 );
	echoBLayout->addWidget( echoSurroundB, 3, 0, 1, 2 );

	QLabel *eqQualityL = new QLabel( tr( "Jakość korektora dźwięku" ) + ": " );

	eqQualityB = new QComboBox;
	eqQualityB->addItems( QStringList()
		<< tr( "Niska" ) + ", " + tr( "rozmiar filtra" ) + ": 256"
		<< tr( "Niska" ) + ", " + tr( "rozmiar filtra" ) + ": 512"
		<< tr( "Średnia" ) + ", " + tr( "rozmiar filtra" ) + ": 1024"
		<< tr( "Średnia" ) + ", " + tr( "rozmiar filtra" ) + ": 2048"
		<< tr( "Wysoka" ) + ", " + tr( "rozmiar filtra" ) + ": 4096"
		<< tr( "Bardzo wysoka" ) + ", " + tr( "rozmiar filtra" ) + ": 8192"
		<< tr( "Bardzo wysoka" ) + ", " + tr( "rozmiar filtra" ) + ": 16384"
		<< tr( "Bardzo wysoka" ) + ", " + tr( "rozmiar filtra" ) + ": 32768"
		<< tr( "Bardzo wysoka" ) + ", " + tr( "rozmiar filtra" ) + ": 65536"
	);
	eqQualityB->setCurrentIndex( sets().getInt( "Equalizer/nbits" ) - 8 );

	QLabel *eqSlidersL = new QLabel( tr( "Liczba suwaków w korektorze dźwięku" ) + ": " );

	eqSlidersB = new QSpinBox;
	eqSlidersB->setRange( 2, 20 );
	eqSlidersB->setValue( sets().getInt( "Equalizer/count" ) );

	eqMinFreqB = new QSpinBox;
	eqMinFreqB->setPrefix( tr( "Minimalna częstotliwość" ) + ": " );
	eqMinFreqB->setSuffix( " Hz" );
	eqMinFreqB->setRange( 10, 300 );
	eqMinFreqB->setValue( sets().getInt( "Equalizer/minFreq" ) );

	eqMaxFreqB = new QSpinBox;
	eqMaxFreqB->setPrefix( tr( "Maksymalna częstotliwość" ) + ": " );
	eqMaxFreqB->setSuffix( " Hz" );
	eqMaxFreqB->setRange( 10000, 96000 );
	eqMaxFreqB->setValue( sets().getInt( "Equalizer/maxFreq" ) );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( voiceRemovalEB, 0, 0, 1, 2 );
	layout->addWidget( phaseReverseEB, 1, 0, 1, 2 );
	layout->addWidget( phaseReverseRightB, 2, 0, 1, 2 );
	layout->addWidget( echoB, 3, 0, 1, 2 );
	layout->addWidget( eqQualityL, 4, 0, 1, 1 );
	layout->addWidget( eqQualityB, 4, 1, 1, 1 );
	layout->addWidget( eqSlidersL, 5, 0, 1, 1 );
	layout->addWidget( eqSlidersB, 5, 1, 1, 1 );
	layout->addWidget( eqMinFreqB, 6, 0, 1, 1 );
	layout->addWidget( eqMaxFreqB, 6, 1, 1, 1 );
}

void ModuleSettingsWidget::voiceRemovalToggle()
{
	sets().set( "VoiceRemoval", voiceRemovalEB->isChecked() );
	SetInstance< VoiceRemoval >();
}
void ModuleSettingsWidget::phaseReverse()
{
	sets().set( "PhaseReverse", phaseReverseEB->isChecked() );
	sets().set( "PhaseReverse/ReverseRight", phaseReverseRightB->isChecked() );
	phaseReverseRightB->setEnabled( phaseReverseEB->isChecked() );
	SetInstance< PhaseReverse >();
}
void ModuleSettingsWidget::echo()
{
	sets().set( "Echo", echoB->isChecked() );
	sets().set( "Echo/Delay", echoDelayB->value() );
	sets().set( "Echo/Volume", echoVolumeB->value() );
	sets().set( "Echo/Feedback", echoFeedbackB->value() );
	sets().set( "Echo/Surround", echoSurroundB->isChecked() );
	SetInstance< Echo >();
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "Equalizer/nbits", eqQualityB->currentIndex() + 8 );
	sets().set( "Equalizer/count", eqSlidersB->value() );
	sets().set( "Equalizer/minFreq", eqMinFreqB->value() );
	sets().set( "Equalizer/maxFreq", eqMaxFreqB->value() );
}
