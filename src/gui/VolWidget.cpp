#include <VolWidget.hpp>

#include <Functions.hpp>

#include <QVBoxLayout>

/**/

VolWidget::VolWidget(int max) :
	menu(this)
{
	setContextMenuPolicy(Qt::CustomContextMenu);

	QVBoxLayout *volL = new QVBoxLayout(this);
	volL->setMargin(0);

	for (int i = 0; i < 2; ++i)
	{
		vol[i].setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed));
		vol[i].setMaximum(qMax(100, max));
		vol[i].setValue(100);
		volL->addWidget(&vol[i]);
		connect(&vol[i], SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
	}

	splitA = menu.addAction(tr("Split channels"));
	splitA->setCheckable(true);
	splitA->setChecked(true);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
	connect(splitA, SIGNAL(triggered(bool)), this, SLOT(splitTriggered(bool)));
}

void VolWidget::setVolume(int volL, int volR, bool first)
{
	vol[0].setValue(volL);
	vol[1].setValue(volR);
	if (first)
	{
		setSlidersToolTip();
		if (volL == volR)
			splitA->trigger();
	}
}
void VolWidget::changeVolume(int deltaVol)
{
	vol[0].setValue(vol[0].value() + deltaVol);
	if (vol[1].isVisible())
		vol[1].setValue(vol[1].value() + deltaVol);
}

void VolWidget::customContextMenuRequested(const QPoint &pos)
{
	menu.popup(mapToGlobal(pos));
}
void VolWidget::splitTriggered(bool splitted)
{
	vol[1].setVisible(splitted);
	if (!splitted)
	{
		const int v = (vol[0].value() + vol[1].value()) / 2;
		for (int i = 0; i < 2; ++i)
			vol[i].setValue(v);
	}
}
void VolWidget::setMaximumVolume(int max)
{
	for (int i = 0; i < 2; ++i)
		vol[i].setMaximum(max);
}

void VolWidget::sliderValueChanged(int v)
{
	Slider *senderSlider = qobject_cast<Slider *>(sender());
	if (senderSlider)
	{
		if (!vol[1].isVisible())
		{
			vol[1].setValue(v);
			if (senderSlider == &vol[1])
				return;
		}
		setSlidersToolTip();
		emit volumeChanged(volumeL(), volumeR());
	}
}

void VolWidget::setSlidersToolTip()
{
	for (int i = 0; i < 2; ++i)
	{
		const double v = vol[i].value() / 100.0;
		vol[i].setToolTip(Functions::dBStr(v * v));
	}
}
