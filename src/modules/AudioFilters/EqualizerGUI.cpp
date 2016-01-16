#include <EqualizerGUI.hpp>
#include <Equalizer.hpp>

#include <Functions.hpp>

#include <QLabel>
#include <QSlider>
#include <QPainter>
#include <QCheckBox>
#include <QGridLayout>
#include <QToolButton>

GraphW::GraphW() :
	preamp(0.5f)
{
	setAutoFillBackground(true);
	setPalette(Qt::black);
}

void GraphW::setValue(int idx, float val)
{
	if (idx == -1) //Preamp
		preamp = val;
	else if (values.size() > idx)
		values[idx] = val;
	update();
}

void GraphW::paintEvent(QPaintEvent *)
{
	if (width() >= 2)
	{
		const QVector< float > graph = Equalizer::interpolate(values, width());

		QPainter p(this);
		p.scale(1.0, height()-1.0);

		QPainterPath path;
		path.moveTo(QPointF(0.0, 1.0 - graph[0]));
		for (int i = 1; i < graph.count(); ++i)
			path.lineTo(i, 1.0 - graph[i]);

		p.setPen(QPen(QColor(102, 51, 128), 0.0));
		p.drawLine(QLineF(0, preamp, width(), preamp));

		p.setPen(QPen(QColor(102, 179, 102), 0.0));
		p.drawPath(path);
	}
}

/**/

EqualizerGUI::EqualizerGUI(Module &module) :
	slidersW(NULL)
{
	dw = new DockWidget;
	dw->setObjectName(EqualizerGUIName);
	dw->setWindowTitle(tr("Equalizer"));
	dw->setWidget(this);

	QCheckBox *enabledB = new QCheckBox;

	QFrame *frame = new QFrame;
	frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	frame->setMaximumHeight(90);
	frame->setMinimumHeight(30);
	frame->setFrameShadow(QFrame::Sunken);
	frame->setFrameShape(QFrame::StyledPanel);
	QGridLayout *graphLayout = new QGridLayout(frame);
	graphLayout->addWidget(&graph);
	graphLayout->setMargin(2);

	QWidget *buttonsW = new QWidget;
	QToolButton *maxB = new QToolButton;
	QToolButton *resetB = new QToolButton;
	QToolButton *minB = new QToolButton;
	maxB->setObjectName("maxB");
	maxB->setArrowType(Qt::RightArrow);
	resetB->setObjectName("resetB");
	resetB->setArrowType(Qt::RightArrow);
	minB->setObjectName("minB");
	minB->setArrowType(Qt::RightArrow);
	connect(maxB, SIGNAL(clicked()), this, SLOT(setSliders()));
	connect(resetB, SIGNAL(clicked()), this, SLOT(setSliders()));
	connect(minB, SIGNAL(clicked()), this, SLOT(setSliders()));
	QVBoxLayout *buttonsLayout = new QVBoxLayout(buttonsW);
	buttonsLayout->addWidget(maxB);
	buttonsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
	buttonsLayout->addWidget(resetB);
	buttonsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
	buttonsLayout->addWidget(minB);
	buttonsLayout->addWidget(new QLabel("\n"));

	layout = new QGridLayout(this);
	layout->addWidget(enabledB, 0, 0, 1, 2);
	layout->addWidget(frame, 1, 0, 1, 2);
	layout->addWidget(buttonsW, 2, 0, 1, 1);

	SetModule(module);

	enabledB->setText(tr("Włączony"));
	enabledB->setChecked(sets().getBool("Equalizer"));
	connect(enabledB, SIGNAL(clicked(bool)), this, SLOT(enabled(bool)));

	connect(dw, SIGNAL(visibilityChanged(bool)), enabledB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), maxB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), resetB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), minB, SLOT(setEnabled(bool)));

	connect(&QMPlay2Core, SIGNAL(wallpaperChanged(bool, double)), this, SLOT(wallpaperChanged(bool, double)));
}

DockWidget *EqualizerGUI::getDockWidget()
{
	return dw;
}

void EqualizerGUI::wallpaperChanged(bool hasWallpaper, double alpha)
{
	QColor c = Qt::black;
	if (hasWallpaper)
		c.setAlphaF(alpha);
	graph.setPalette(c);
}
void EqualizerGUI::enabled(bool b)
{
	sets().set("Equalizer", b);
	SetInstance< Equalizer >();
}
void EqualizerGUI::valueChanged(int v)
{
	QSlider *slider = qobject_cast< QSlider * >(sender());
	if (slider)
	{
		graph.setValue(slider->property("idx").toInt(), v / 100.0f);
		sets().set("Equalizer/" + slider->property("idx").toString(), v);
		slider->setToolTip(Functions::dBStr(v / 50.0));
		SetInstance< Equalizer >();
	}
}
void EqualizerGUI::setSliders()
{
	graph.hide();
	foreach (QObject *o, slidersW->children())
	{
		QSlider *slider = qobject_cast< QSlider * >(o);
		if (slider)
		{
			const bool isPreamp = slider->property("preamp").toBool();
			if (sender()->objectName() == "maxB" && !isPreamp)
				slider->setValue(slider->maximum() - 3);
			else if (sender()->objectName() == "resetB")
				slider->setValue(slider->maximum() / 2);
			else if (sender()->objectName() == "minB" && !isPreamp)
				slider->setValue(slider->minimum() + 3);
		}
	}
	graph.show();
}

bool EqualizerGUI::set()
{
	delete slidersW;

	slidersW = new QWidget;
	slidersW->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	QGridLayout *slidersLayout = new QGridLayout(slidersW);

	QVector< float > freqs = Equalizer::freqs(sets());
	graph.setValues(freqs.count());
	for (int i = -1; i < freqs.count(); ++i)
	{
		QSlider *slider = new QSlider;
		slider->setMaximum(100);
		slider->setTickInterval(50);
		slider->setProperty("idx", i);
		slider->setTickPosition(QSlider::TicksBelow);
		slider->setValue(sets().getInt("Equalizer/" + QString::number(i)));
		slider->setToolTip(Functions::dBStr(slider->value() / 50.0));
		connect(slider, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

		graph.setValue(i, slider->value() / 100.0f);

		QLabel *descrL = new QLabel;
		descrL->setAlignment(Qt::AlignCenter);

		if (i == -1)
		{
			descrL->setText(tr("Pre\namp"));
			slider->setProperty("preamp", true);
		}
		else if (freqs[i] < 1000.0f)
			descrL->setText(QString::number(freqs[i], 'f', 0) + "\nHz");
		else
			descrL->setText(QString::number(freqs[i] / 1000.0f, 'g', 2) + "k\nHz");

		slidersLayout->addWidget(slider, 0, i+1);
		slidersLayout->addWidget(descrL, 1, i+1);
	}

	layout->addWidget(slidersW, 2, 1, 1, 1);
	connect(dw, SIGNAL(visibilityChanged(bool)), slidersW, SLOT(setEnabled(bool)));

	return true;
}
