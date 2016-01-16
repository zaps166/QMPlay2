#include <VideoEqualizer.hpp>

#include <QMPlay2Core.hpp>
#include <Settings.hpp>
#include <Slider.hpp>

#include <QGridLayout>
#include <QPushButton>
#include <QVariant>
#include <QLabel>

VideoEqualizer::VideoEqualizer()
{
	layout = new QGridLayout;

	int i;
	for ( i = BRIGHTNESS; i < CONTROLS_COUNT; ++i )
	{
		QLabel *&titleL = controls[i].titleL;
		QLabel *&valueL = controls[i].valueL;
		Slider *&slider = controls[i].slider;

		switch ( i )
		{
			case BRIGHTNESS:
				titleL = new QLabel( tr( "Jasność" ) + ": " );
				break;
			case SATURATION:
				titleL = new QLabel( tr( "Nasycenie" ) + ": " );
				break;
			case CONTRAST:
				titleL = new QLabel( tr( "Kontrast" ) + ": " );
				break;
			case HUE:
				titleL = new QLabel( tr( "Odcień" ) + ": " );
				break;
		}

		valueL = new QLabel( "0" );

		slider = new Slider;
		slider->setProperty( "valueL", qVariantFromValue( ( void * )valueL ) );
		slider->setTickPosition( QSlider::TicksBelow );
		slider->setTickInterval( 25 );
		slider->setMinimum( -100 );
		slider->setMaximum( 100 );
		slider->setValue( 0 );

		layout->addWidget( titleL, i, 0 );
		layout->addWidget( slider, i, 1 );
		layout->addWidget( valueL, i, 2 );

		connect( slider, SIGNAL( valueChanged( int ) ), this, SLOT( setValue( int ) ) );
	}

	resetB = new QPushButton( tr( "Resetuj" ) );
	connect( resetB, SIGNAL( clicked() ), this, SLOT( reset() ) );

	layout->addWidget( resetB, i++, 0, 1, 3 );
	layout->addItem( new QSpacerItem( 40, 0, QSizePolicy::Expanding, QSizePolicy::Minimum ), i, 2 );

	setLayout( layout );
}

void VideoEqualizer::restoreValues()
{
	controls[ BRIGHTNESS ].slider->setValue( QMPlay2Core.getSettings().getInt( "VideoEqualizer/Brightness" ) );
	controls[ SATURATION ].slider->setValue( QMPlay2Core.getSettings().getInt( "VideoEqualizer/Saturation" ) );
	controls[ CONTRAST ].slider->setValue( QMPlay2Core.getSettings().getInt( "VideoEqualizer/Contrast" ) );
	controls[ HUE ].slider->setValue( QMPlay2Core.getSettings().getInt( "VideoEqualizer/Hue" ) );
}
void VideoEqualizer::saveValues()
{
	QMPlay2Core.getSettings().set( "VideoEqualizer/Brightness", controls[ BRIGHTNESS ].slider->value() );
	QMPlay2Core.getSettings().set( "VideoEqualizer/Saturation", controls[ SATURATION ].slider->value() );
	QMPlay2Core.getSettings().set( "VideoEqualizer/Contrast", controls[ CONTRAST ].slider->value() );
	QMPlay2Core.getSettings().set( "VideoEqualizer/Hue", controls[ HUE ].slider->value() );
}

void VideoEqualizer::setValue( int v )
{
	( ( QLabel * )sender()->property( "valueL" ).value< void * >() )->setText( QString::number( v ) + "" );
	emit valuesChanged
	(
		controls[ BRIGHTNESS ].slider->value(),
		controls[ SATURATION ].slider->value(),
		controls[ CONTRAST ].slider->value(),
		controls[ HUE ].slider->value()
	);
}
void VideoEqualizer::reset()
{
	for ( int i = BRIGHTNESS; i < CONTROLS_COUNT; ++i )
		controls[ i ].slider->setValue( 0 );
}
