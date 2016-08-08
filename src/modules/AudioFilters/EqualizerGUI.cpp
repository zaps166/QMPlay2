/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <EqualizerGUI.hpp>
#include <Equalizer.hpp>

#include <Functions.hpp>

#include <QMenu>
#include <QLabel>
#include <QSlider>
#include <QPainter>
#include <QCheckBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QToolButton>
#include <QInputDialog>

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
		const QVector<float> graph = Equalizer::interpolate(values, width());

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

EqualizerGUI::EqualizerGUI(Module &module)
{
	dw = new DockWidget;
	dw->setObjectName(EqualizerGUIName);
	dw->setWindowTitle(tr("Equalizer"));
	dw->setWidget(this);

	deletePresetMenu = new QMenu(this);
	connect(deletePresetMenu->addAction(tr("Delete")), SIGNAL(triggered()), this, SLOT(deletePreset()));

	QWidget *headerW = new QWidget;

	presetsMenu = new QMenu(this);
	presetsMenu->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(presetsMenu, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(deletePresetMenuRequest(QPoint)));
	QAction *addAct = presetsMenu->addAction(tr("Add new preset"));
	addAct->setObjectName("resetA");
	connect(addAct, SIGNAL(triggered()), this, SLOT(addPreset()));
	presetsMenu->addSeparator();

	enabledB = new QCheckBox;

	QToolButton *presetsB = new QToolButton;
	presetsB->setPopupMode(QToolButton::InstantPopup);
	presetsB->setText(tr("Presets"));
	presetsB->setAutoRaise(true);
	presetsB->setMenu(presetsMenu);

	QToolButton *showSettingsB = new QToolButton;
	showSettingsB->setIcon(QIcon(":/settings"));
	showSettingsB->setIcon(QMPlay2Core.getIconFromTheme("configure"));
	showSettingsB->setToolTip(tr("Settings"));
	showSettingsB->setAutoRaise(true);
	connect(showSettingsB, SIGNAL(clicked()), this, SLOT(showSettings()));

	QHBoxLayout *headerLayout = new QHBoxLayout(headerW);
	headerLayout->addWidget(enabledB);
	headerLayout->addWidget(presetsB);
	headerLayout->addWidget(showSettingsB);
	headerLayout->setMargin(0);

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

	slidersA = new QScrollArea;
	slidersA->setWidgetResizable(true);
	slidersA->setFrameShape(QFrame::NoFrame);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(headerW, 0, 0, 1, 2);
	layout->addWidget(frame, 1, 0, 1, 2);
	layout->addWidget(buttonsW, 2, 0, 1, 1);
	layout->addWidget(slidersA, 2, 1, 1, 1);

	SetModule(module);

	enabledB->setText(tr("ON"));
	enabledB->setChecked(sets().getBool("Equalizer"));
	connect(enabledB, SIGNAL(clicked(bool)), this, SLOT(enabled(bool)));

	connect(dw, SIGNAL(visibilityChanged(bool)), enabledB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), presetsB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), showSettingsB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), maxB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), resetB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), minB, SLOT(setEnabled(bool)));
	connect(dw, SIGNAL(visibilityChanged(bool)), slidersA, SLOT(setEnabled(bool)));

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
	setInstance<Equalizer>();
}
void EqualizerGUI::valueChanged(int v)
{
	QSlider *slider = qobject_cast<QSlider *>(sender());
	if (slider)
	{
		graph.setValue(slider->property("idx").toInt(), v / 100.0f);
		sets().set("Equalizer/" + slider->property("idx").toString(), v);
		slider->setToolTip(Functions::dBStr(v / 50.0));
		setInstance<Equalizer>();
	}
}
void EqualizerGUI::setSliders()
{
	const QString objectName = sender()->objectName();
	graph.hide();
	foreach (QObject *o, slidersA->widget()->children())
	{
		if (QSlider *slider = qobject_cast<QSlider *>(o))
		{
			const bool isPreamp = slider->property("preamp").toBool();
			if (objectName == "maxB" && !isPreamp)
				slider->setValue(slider->maximum() - 3);
			else if (objectName == "resetB")
				slider->setValue(slider->maximum() / 2);
			else if (objectName == "minB" && !isPreamp)
				slider->setValue(slider->minimum() + 3);
		}
	}
	graph.show();
}

void EqualizerGUI::addPreset()
{
	bool ok = false;
	QString name = QInputDialog::getText(this, tr("New preset"), tr("Enter new preset name"), QLineEdit::Normal, QString(), &ok).simplified();
	if (ok && !name.isEmpty())
	{
		QStringList presetsList = sets().getStringList("Equalizer/Presets");
		if (!presetsList.contains(name))
		{
			presetsList.append(name);
			sets().set("Equalizer/Presets", presetsList);
		}

		QMap<int, int> values;
		foreach (QObject *o, slidersA->widget()->children())
		{
			if (QSlider *slider = qobject_cast<QSlider *>(o))
			{
				const bool isPreamp = slider->property("preamp").toBool();
				if (isPreamp)
					values[-1] = slider->value();
				else
					values[slider->property("idx").toInt()] = slider->value();
			}
		}
		QByteArray dataArr;
		QDataStream stream(&dataArr, QIODevice::WriteOnly);
		stream << values;
		sets().set("Equalizer/Preset" + name, dataArr.toBase64().data());

		loadPresets();
	}
}

void EqualizerGUI::showSettings()
{
	emit QMPlay2Core.showSettings("AudioFilters");
}

void EqualizerGUI::deletePresetMenuRequest(const QPoint &p)
{
	QAction *presetAct = presetsMenu->actionAt(p);
	if (presetAct && presetsMenu->actions().indexOf(presetAct) >= 2)
	{
		deletePresetMenu->setProperty("presetAct", QVariant::fromValue((void *)presetAct));
		deletePresetMenu->popup(presetsMenu->mapToGlobal(p));
	}
}
void EqualizerGUI::deletePreset()
{
	if (QAction *act = (QAction *)deletePresetMenu->property("presetAct").value<void *>())
	{
		QStringList presetsList = sets().getStringList("Equalizer/Presets");
		presetsList.removeOne(act->text());

		if (presetsList.isEmpty())
			sets().remove("Equalizer/Presets");
		else
			sets().set("Equalizer/Presets", presetsList);
		sets().remove("Equalizer/Preset" + act->text());

		delete act;
	}
}

void EqualizerGUI::setPresetValues()
{
	if (QAction *act = qobject_cast<QAction *>(sender()))
	{
		QMap<int, int> values = getPresetValues(act->text());
		if (values.count() > 1)
		{
			foreach (QObject *o, slidersA->widget()->children())
			{
				if (QSlider *slider = qobject_cast<QSlider *>(o))
				{
					const bool isPreamp = slider->property("preamp").toBool();
					if (isPreamp)
						slider->setValue(values.value(-1));
					else
						slider->setValue(values.value(slider->property("idx").toInt()));
				}
			}
			if (!enabledB->isChecked())
				enabledB->click();
		}
	}
}

bool EqualizerGUI::set()
{
	delete slidersA->widget();

	QWidget *slidersW = new QWidget;
	slidersW->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	QGridLayout *slidersLayout = new QGridLayout(slidersW);

	const QVector<float> freqs = Equalizer::freqs(sets());
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

	slidersA->setWidget(slidersW);

	loadPresets();

	return true;
}

void EqualizerGUI::loadPresets()
{
	const QList<QAction *> presetsActions = presetsMenu->actions();
	for (int i = 2; i < presetsActions.count(); ++i)
		delete presetsActions[i];

	const int count = sets().getInt("Equalizer/count");

	QStringList presetsList = sets().getStringList("Equalizer/Presets");
	QVector<int> presetsToRemove;
	for (int i = 0; i < presetsList.count(); ++i)
	{
		const int sliders = getPresetValues(presetsList.at(i)).count() - 1;
		if (sliders < 1)
			presetsToRemove.append(i);
		else
		{
			QAction *presetAct = presetsMenu->addAction(presetsList.at(i));
			connect(presetAct, SIGNAL(triggered()), this, SLOT(setPresetValues()));
			presetAct->setEnabled(sliders == count);
		}
	}

	if (!presetsToRemove.isEmpty())
	{
		for (int i = presetsToRemove.count() - 1; i >= 0; --i)
		{
			const int idxToRemove = presetsToRemove.at(i);
			sets().remove("Equalizer/Preset" + presetsList.at(idxToRemove));
			presetsList.removeAt(idxToRemove);
		}
		if (presetsList.isEmpty())
			sets().remove("Equalizer/Presets");
		else
			sets().set("Equalizer/Presets", presetsList);
	}

	deletePresetMenu->setProperty("presetAct", QVariant());
}

QMap<int, int> EqualizerGUI::getPresetValues(const QString &name)
{
	QByteArray dataArr = QByteArray::fromBase64(sets().getByteArray("Equalizer/Preset" + name));
	QDataStream stream(&dataArr, QIODevice::ReadOnly);
	QMap<int, int> values;
	stream >> values;
	return values;
}
