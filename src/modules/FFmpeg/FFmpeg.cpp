#include <FFmpeg.hpp>
#include <FFDemux.hpp>
#include <FFDecSW.hpp>
#ifdef QMPlay2_VAAPI
	#include <FFDecVAAPI.hpp>
	#include <VAAPIWriter.hpp>
#endif
#ifdef QMPlay2_VDPAU
	#include <FFDecVDPAU.hpp>
	#ifdef QMPlay2_VDPAU_NW
		#include <FFDecVDPAU_NW.hpp>
	#endif
	#include <VDPAUWriter.hpp>
#endif
#include <FFReader.hpp>
#include <FFCommon.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
#ifdef QMPlay2_libavdevice
	#include <libavdevice/avdevice.h>
#endif
}

FFmpeg::FFmpeg() :
	Module( "FFmpeg" )
{
	moduleImg = QImage( ":/FFmpeg" );

	init( "DemuxerEnabled", true );
	init( "DecoderEnabled", true );
#ifdef QMPlay2_VDPAU
	init( "DecoderVDPAUEnabled", true );
	init( "VDPAUDeintMethod", 1 );
	if ( getUInt( "VDPAUDeintMethod" ) > 2 )
		set( "VDPAUDeintMethod", 1 );
	init( "VDPAUNoiseReductionEnabled", false );
	init( "VDPAUNoiseReductionLvl", 0.0 );
	init( "VDPAUSharpnessEnabled", false );
	init( "VDPAUSharpnessLvl", 0.0 );
	init( "VDPAUHQScaling", 0 );
	if ( getUInt( "VDPAUHQScaling" ) > 9 )
		set( "VDPAUHQScaling", 0 );
#endif
#ifdef QMPlay2_VAAPI
	init( "AllowVDPAUinVAAPI", false );
	init( "DecoderVAAPIEnabled", true );
	init( "VAAPIDeintMethod", 1 );
	if ( getUInt( "VAAPIDeintMethod" ) > 2 )
		set( "VAAPIDeintMethod", 1 );
#endif
	init( "HurryUP", true );
	init( "SkipFrames", true );
	init( "ForceSkipFrames", false );
	init( "Threads", 0 );
	init( "LowresValue", 0 );
	init( "ThreadTypeSlice", false );

	static bool firstTime = true;
	if ( firstTime )
	{
#ifndef QT_DEBUG
		av_log_set_level( AV_LOG_FATAL );
#endif
		av_register_all();
#ifdef QMPlay2_libavdevice
		avdevice_register_all();
#endif
		firstTime = false;
	}
	avformat_network_init();
}
FFmpeg::~FFmpeg()
{
	avformat_network_deinit();
}

QList< FFmpeg::Info > FFmpeg::getModulesInfo( const bool showDisabled ) const
{
	QList< Info > modulesInfo;
	if ( showDisabled || getBool( "DemuxerEnabled" ) )
		modulesInfo += Info( DemuxerName, DEMUXER );
	if ( showDisabled || getBool( "DecoderEnabled" ) )
		modulesInfo += Info( DecoderName, DECODER );
#ifdef QMPlay2_VDPAU
	if ( showDisabled || getBool( "DecoderVDPAUEnabled" ) )
	{
		modulesInfo += Info( DecoderVDPAUName, DECODER );
		modulesInfo += Info( VDPAUWriterName, WRITER );
#ifdef QMPlay2_VDPAU_NW
		modulesInfo += Info( DecoderVDPAU_NWName, DECODER );
#endif
	}
#endif
#ifdef QMPlay2_VAAPI
	if ( showDisabled || getBool( "DecoderVAAPIEnabled" ) )
	{
		modulesInfo += Info( DecoderVAAPIName, DECODER );
		modulesInfo += Info( VAAPIWriterName, WRITER );
	}
#endif
	modulesInfo += Info( FFReaderName, READER, QStringList() << "file" << "http" << "https" << "mms" << "rtmp" );
	return modulesInfo;
}
void *FFmpeg::createInstance( const QString &name )
{
	if ( name == DemuxerName && getBool( "DemuxerEnabled" ) )
		return new FFDemux( mutex, *this );
	else if ( name == DecoderName && getBool( "DecoderEnabled" ) )
		return new FFDecSW( mutex, *this );
#ifdef QMPlay2_VDPAU
	else if ( name == DecoderVDPAUName && getBool( "DecoderVDPAUEnabled" ) )
		return new FFDecVDPAU( mutex, *this );
#ifdef QMPlay2_VDPAU_NW
	else if ( name == DecoderVDPAU_NWName && getBool( "DecoderVDPAUEnabled" ) )
		return new FFDecVDPAU_NW( mutex, *this );
#endif
#endif
#ifdef QMPlay2_VAAPI
	else if ( name == DecoderVAAPIName && getBool( "DecoderVAAPIEnabled" ) )
		return new FFDecVAAPI( mutex, *this );
#endif
	else if ( name == FFReaderName )
		return new FFReader;
	return NULL;
}

FFmpeg::SettingsWidget *FFmpeg::getSettingsWidget()
{
	return new ModuleSettingsWidget( *this );
}

QMPLAY2_EXPORT_PLUGIN( FFmpeg )

/**/

#include <Slider.hpp>

#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget( Module &module ) :
	Module::SettingsWidget( module )
{
	demuxerEB = new QCheckBox( tr( "Demuxer" ) );
	demuxerEB->setChecked( sets().getBool( "DemuxerEnabled" ) );

	decoderB = new QGroupBox( tr( "Dekoder programowy" ) );
	decoderB->setCheckable( true );
	decoderB->setChecked( sets().getBool( "DecoderEnabled" ) );

#ifdef QMPlay2_VDPAU
	decoderVDPAUB = new QGroupBox( tr( "Dekoder" ) + " VDPAU - " + tr( "dekodowanie sprzętowe" ) );
	decoderVDPAUB->setCheckable( true );
	decoderVDPAUB->setChecked( sets().getBool( "DecoderVDPAUEnabled" ) );

	QLabel *vdpauDeintMethodL = new QLabel( tr( "Poprawa jakości usuwania przeplotu" ) + ": " );

	vdpauDeintMethodB = new QComboBox;
	vdpauDeintMethodB->addItems( QStringList() << tr( "Brak" ) << "Temporal" << "Temporal-spatial" );
	vdpauDeintMethodB->setCurrentIndex( sets().getInt( "VDPAUDeintMethod" ) );
	if ( vdpauDeintMethodB->currentIndex() < 0 )
		vdpauDeintMethodB->setCurrentIndex( 1 );

	QLabel *vdpauHQScalingL = new QLabel( tr( "Poziom skalowania obrazu" ) + ": " );

	vdpauHQScalingB = new QComboBox;
	for ( int i = 0; i <= 9; ++i )
		vdpauHQScalingB->addItem( QString( "Level %1" ).arg( i ) );
	vdpauHQScalingB->setCurrentIndex( sets().getUInt( "VDPAUHQScaling" ) );

	noisereductionVDPAUB = new QCheckBox( tr( "Redukcja szumu" ) );
	noisereductionVDPAUB->setChecked( sets().getBool( "VDPAUNoiseReductionEnabled" ) );
	connect( noisereductionVDPAUB, SIGNAL( clicked() ), this, SLOT( checkEnables() ) );
	connect( noisereductionVDPAUB, SIGNAL( clicked() ), this, SLOT( setVDPAU() ) );
	noisereductionLvlVDPAUS = new Slider;
	noisereductionLvlVDPAUS->setRange( 0, 50 );
	noisereductionLvlVDPAUS->setTickInterval( 50 );
	noisereductionLvlVDPAUS->setTickPosition( QSlider::TicksBelow );
	noisereductionLvlVDPAUS->setValue( sets().getDouble( "VDPAUNoiseReductionLvl" ) * 50 );
	connect( noisereductionLvlVDPAUS, SIGNAL( valueChanged( int ) ), this, SLOT( setVDPAU() ) );

	sharpnessVDPAUB = new QCheckBox( tr( "Ostrość" ) );
	sharpnessVDPAUB->setChecked( sets().getBool( "VDPAUSharpnessEnabled" ) );
	connect( sharpnessVDPAUB, SIGNAL( clicked() ), this, SLOT( checkEnables() ) );
	connect( sharpnessVDPAUB, SIGNAL( clicked() ), this, SLOT( setVDPAU() ) );
	sharpnessLvlVDPAUS = new Slider;
	sharpnessLvlVDPAUS->setRange( -50, 50 );
	sharpnessLvlVDPAUS->setTickInterval( 50 );
	sharpnessLvlVDPAUS->setTickPosition( QSlider::TicksBelow );
	sharpnessLvlVDPAUS->setValue( sets().getDouble( "VDPAUSharpnessLvl" ) * 50 );
	connect( sharpnessLvlVDPAUS, SIGNAL( valueChanged( int ) ), this, SLOT( setVDPAU() ) );

	checkEnables();

	QGridLayout *vdpauLayout = new QGridLayout( decoderVDPAUB );
	vdpauLayout->addWidget( vdpauDeintMethodL, 0, 0, 1, 1 );
	vdpauLayout->addWidget( vdpauDeintMethodB, 0, 1, 1, 1 );
	vdpauLayout->addWidget( vdpauHQScalingL, 1, 0, 1, 1 );
	vdpauLayout->addWidget( vdpauHQScalingB, 1, 1, 1, 1 );
	vdpauLayout->addWidget( noisereductionVDPAUB, 2, 0, 1, 1 );
	vdpauLayout->addWidget( noisereductionLvlVDPAUS, 2, 1, 1, 1 );
	vdpauLayout->addWidget( sharpnessVDPAUB, 3, 0, 1, 1 );
	vdpauLayout->addWidget( sharpnessLvlVDPAUS, 3, 1, 1, 1 );
#endif

#ifdef QMPlay2_VAAPI
	decoderVAAPIEB = new QGroupBox( tr( "Dekoder" ) + " VAAPI - " + tr( "dekodowanie sprzętowe" ) );
	decoderVAAPIEB->setCheckable( true );
	decoderVAAPIEB->setChecked( sets().getBool( "DecoderVAAPIEnabled" ) );

	allowVDPAUinVAAPIB = new QCheckBox( tr( "Zezwalaj na VDPAU" ) );
	allowVDPAUinVAAPIB->setChecked( sets().getBool( "AllowVDPAUinVAAPI" ) );

	QLabel *vaapiDeintMethodL = new QLabel( tr( "Poprawa jakości usuwania przeplotu" ) + ": " );

	vaapiDeintMethodB = new QComboBox;
	vaapiDeintMethodB->addItems( QStringList() << tr( "Brak" ) << "Motion adaptive" << "Motion compensated" );
	vaapiDeintMethodB->setCurrentIndex( sets().getInt( "VAAPIDeintMethod" ) );
	if ( vaapiDeintMethodB->currentIndex() < 0 )
		vaapiDeintMethodB->setCurrentIndex( 1 );

	QGridLayout *vaapiLayout = new QGridLayout( decoderVAAPIEB );
	vaapiLayout->addWidget( allowVDPAUinVAAPIB, 0, 0, 1, 2 );
	vaapiLayout->addWidget( vaapiDeintMethodL, 1, 0, 1, 1 );
	vaapiLayout->addWidget( vaapiDeintMethodB, 1, 1, 1, 1 );

	#ifndef HAVE_VPP
		vaapiDeintMethodL->setEnabled( false );
		vaapiDeintMethodB->setEnabled( false );
	#endif
#endif

	/* Pospiesz się */
	hurryUpB = new QGroupBox( tr( "Pospiesz się" ) );
	hurryUpB->setCheckable( true );
	hurryUpB->setChecked( sets().getBool( "HurryUP" ) );

	skipFramesB = new QCheckBox( tr( "Pomijaj niektóre klatki" ) );
	skipFramesB->setChecked( sets().getBool( "SkipFrames" ) );

	forceSkipFramesB = new QCheckBox( tr( "Wymuś pomijanie niektórych klatek" ) );
	forceSkipFramesB->setChecked( sets().getBool( "ForceSkipFrames" ) );

	QHBoxLayout *hurryUpLayout = new QHBoxLayout( hurryUpB );
	hurryUpLayout->addWidget( skipFramesB );
	hurryUpLayout->addWidget( forceSkipFramesB );
	/**/

	QLabel *threadsL = new QLabel( tr( "Ilość wątków użytych do dekodowania wideo" ) + ": " );

	threadsB = new QSpinBox;
	threadsB->setRange( 0, 16 );
	threadsB->setSpecialValueText( tr( "Automatycznie" ) );
	threadsB->setValue( sets().getInt( "Threads" ) );

	QLabel *lowresL = new QLabel( tr( "Dekodowanie w niskiej rozdzielczości (niektóre kodeki)" ) + ": " );

	lowresB = new QComboBox;
	lowresB->addItems( QStringList() << tr( "Normalna wielkość" ) << tr( "4x mniejszy" ) << tr( "16x mniejszy" ) );
	lowresB->setCurrentIndex( sets().getInt( "LowresValue" ) );
	if ( lowresB->currentIndex() < 0 )
	{
		lowresB->setCurrentIndex( 0 );
		sets().set( "LowresValue", 0 );
	}

	QLabel *thrTypeL = new QLabel( tr( "Sposób dekodowania wielowątkowego" ) + ": " );

	thrTypeB = new QComboBox;
	thrTypeB->addItems( QStringList() << tr( "Ramki" ) << tr( "Kawałki" ) );
	thrTypeB->setCurrentIndex( sets().getInt( "ThreadTypeSlice" ) );
	if ( thrTypeB->currentIndex() < 0 )
	{
		thrTypeB->setCurrentIndex( 0 );
		sets().set( "ThreadTypeSlice", 0 );
	}

	QGridLayout *decoderLayout = new QGridLayout( decoderB );
	decoderLayout->addWidget( threadsL, 0, 0, 1, 1 );
	decoderLayout->addWidget( threadsB, 0, 1, 1, 1 );
	decoderLayout->addWidget( lowresL, 1, 0, 1, 1 );
	decoderLayout->addWidget( lowresB, 1, 1, 1, 1 );
	decoderLayout->addWidget( thrTypeL, 2, 0, 1, 1 );
	decoderLayout->addWidget( thrTypeB, 2, 1, 1, 1 );
	decoderLayout->addWidget( hurryUpB, 3, 0, 1, 2 );

	connect( skipFramesB, SIGNAL( clicked( bool ) ), forceSkipFramesB, SLOT( setEnabled( bool ) ) );
	if ( hurryUpB->isChecked() || !skipFramesB->isChecked() )
		forceSkipFramesB->setEnabled( skipFramesB->isChecked() );

	QGridLayout *layout = new QGridLayout( this );
	layout->addWidget( demuxerEB );
#ifdef QMPlay2_VDPAU
	layout->addWidget( decoderVDPAUB );
#endif
#ifdef QMPlay2_VAAPI
	layout->addWidget( decoderVAAPIEB );
#endif
	layout->addWidget( decoderB );
}

#ifdef QMPlay2_VDPAU
void ModuleSettingsWidget::setVDPAU()
{
	sets().set( "VDPAUNoiseReductionEnabled", noisereductionVDPAUB->isChecked() );
	sets().set( "VDPAUNoiseReductionLvl", noisereductionLvlVDPAUS->value() / 50.0 );
	sets().set( "VDPAUSharpnessEnabled", sharpnessVDPAUB->isChecked() );
	sets().set( "VDPAUSharpnessLvl", sharpnessLvlVDPAUS->value() / 50.0 );
	SetInstance< VDPAUWriter >();
}
void ModuleSettingsWidget::checkEnables()
{
	noisereductionLvlVDPAUS->setEnabled( noisereductionVDPAUB->isChecked() );
	sharpnessLvlVDPAUS->setEnabled( sharpnessVDPAUB->isChecked() );
}
#endif

void ModuleSettingsWidget::saveSettings()
{
	sets().set( "DemuxerEnabled", demuxerEB->isChecked() );
	sets().set( "DecoderEnabled", decoderB->isChecked() );
	sets().set( "HurryUP", hurryUpB ->isChecked() );
	sets().set( "SkipFrames", skipFramesB->isChecked() );
	sets().set( "ForceSkipFrames", forceSkipFramesB->isChecked() );
	sets().set( "Threads", threadsB->value() );
	sets().set( "LowresValue", lowresB->currentIndex() );
	sets().set( "ThreadTypeSlice", thrTypeB->currentIndex() );
#ifdef QMPlay2_VDPAU
	sets().set( "DecoderVDPAUEnabled", decoderVDPAUB->isChecked() );
	sets().set( "VDPAUDeintMethod", vdpauDeintMethodB->currentIndex() );
	sets().set( "VDPAUHQScaling", vdpauHQScalingB->currentIndex() );
#endif
#ifdef QMPlay2_VAAPI
	sets().set( "AllowVDPAUinVAAPI", allowVDPAUinVAAPIB->isChecked() );
	sets().set( "DecoderVAAPIEnabled", decoderVAAPIEB->isChecked() );
	sets().set( "VAAPIDeintMethod", vaapiDeintMethodB->currentIndex() );
#endif
}
