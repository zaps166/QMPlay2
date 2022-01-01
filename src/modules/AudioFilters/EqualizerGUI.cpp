/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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
#include <QPainterPath>
#include <QCheckBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QToolButton>
#include <QInputDialog>

template <typename W>
static W *setSmallerFont(W *w)
{
    QFont font = w->font();
    font.setPointSize(qMax(6, font.pointSize() - 2));
    w->setFont(font);
    return w;
}

/**/

GraphW::GraphW() :
    preamp(0.5f)
{
    setAutoFillBackground(true);
    setPalette(Qt::black);
}

void GraphW::setValue(int idx, float val)
{
    if (idx == -1)
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
        p.scale(1.0, height() - 0.5);

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
    canUpdateEqualizer(true)
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
    enabledB->setFocusPolicy(Qt::TabFocus);

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
    autoCheckBoxSpacer = new QWidget;
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
    buttonsLayout->addWidget(autoCheckBoxSpacer);
    buttonsLayout->addWidget(maxB);
    buttonsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    buttonsLayout->addWidget(resetB);
    buttonsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    buttonsLayout->addWidget(minB);
    buttonsLayout->addWidget(setSmallerFont(new QLabel("\n")));
    buttonsLayout->setMargin(0);

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

bool EqualizerGUI::set()
{
    sliders.clear();
    delete slidersA->widget();

    QWidget *slidersW = new QWidget;
    slidersW->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    QHBoxLayout *slidersLayout = new QHBoxLayout(slidersW);
    slidersLayout->setMargin(0);

    const QVector<float> freqs = Equalizer::freqs(sets());
    graph.setValues(freqs.count());
    for (int i = -1; i < freqs.count(); ++i)
    {
        QWidget *sliderW = new QWidget;

        QGridLayout *sliderWLaout = new QGridLayout(sliderW);
        sliderWLaout->setMargin(0);

        const int value = sets().getInt(QString("Equalizer/%1").arg(i));

        QSlider *slider = new QSlider;
        slider->setMaximum(100);
        slider->setTickInterval(50);
        slider->setProperty("idx", i);
        slider->setTickPosition(QSlider::TicksBelow);
        slider->setValue((value < 0) ? ~value : value);
        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

        QLabel *descrL = setSmallerFont(new QLabel("\n"));
        descrL->setAlignment(Qt::AlignCenter);
        descrL->setMinimumWidth(50);

        QCheckBox *checkB = setSmallerFont(new QCheckBox);
        connect(checkB, SIGNAL(clicked(bool)), this, SLOT(sliderChecked(bool)));
        checkB->setFocusPolicy(Qt::TabFocus);
        checkB->setProperty("sliderIdx", i);

        slider->setProperty("label", QVariant::fromValue((void *)descrL));
        slider->setProperty("checkbox", QVariant::fromValue((void *)checkB));

        const bool sliderEnabled = (value >= 0);
        slider->setEnabled(sliderEnabled);

        slidersLayout->addWidget(sliderW);

        if (i == -1)
        {
            checkB->setText(tr("Auto"));
            checkB->setChecked(!sliderEnabled);

            sliderWLaout->addWidget(checkB, 0, 0, 1, 3);

            sliderW->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

            descrL->setText(tr("Preamp") + descrL->text());

            QFrame *line = new QFrame;
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Sunken);
            slidersLayout->addWidget(line);
        }
        else
        {
            checkB->setChecked(sliderEnabled);
            checkB->setText(" ");

            sliderWLaout->addWidget(checkB, 0, 1);

            sliderW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            if (freqs[i] < 1000.0f)
                descrL->setText(QString::number(freqs[i], 'f', 0) + " Hz" + descrL->text());
            else
                descrL->setText(QString::number(freqs[i] / 1000.0f, 'g', 2) + " kHz" + descrL->text());
        }

        sliderWLaout->addWidget(slider, 1, 1);
        sliderWLaout->addWidget(descrL, 2, 0, 1, 3);

        sliders.append(slider);

        setSliderInfo(i, value);
    }

    slidersA->setWidget(slidersW);

    if (getSliderCheckBox(sliders.at(0))->isChecked()) //auto preamp
    {
        canUpdateEqualizer = false;
        autoPreamp();
        canUpdateEqualizer = true;
    }

    loadPresets();

    return true;
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
    if (QSlider *slider = qobject_cast<QSlider *>(sender()))
        sliderValueChanged(slider->property("idx").toInt(), v);
}
void EqualizerGUI::sliderChecked(bool b)
{
    QCheckBox *checkB = (QCheckBox *)sender();

    const int idx = checkB->property("sliderIdx").toInt();
    const bool isPreamp = (idx == -1);

    QSlider *slider = sliders.at(idx + 1);
    slider->setEnabled(b != isPreamp);

    if (!isPreamp)
        sliderValueChanged(idx, b ? slider->value() : ~slider->value());
    else
    {
        if (b)
        {
            sets().set("Equalizer/-1", ~slider->value());
            autoPreamp();
        }
        else
        {
            const int value = sets().getInt("Equalizer/-1");
            slider->setValue(~value);
            sets().set("Equalizer/-1", slider->value());
        }
    }
}

void EqualizerGUI::setSliders()
{
    const QString objectName = sender()->objectName();
    graph.hide();
    for (QSlider *slider : qAsConst(sliders))
    {
        const bool isPreamp = (sliders.at(0) == slider);

        if (objectName == "maxB" && !isPreamp)
            slider->setValue(slider->maximum());
        else if (objectName == "resetB")
            slider->setValue(slider->maximum() / 2);
        else if (objectName == "minB" && !isPreamp)
            slider->setValue(slider->minimum());

        if (!isPreamp)
        {
            QCheckBox *checkB = getSliderCheckBox(slider);
            if (!checkB->isChecked())
                checkB->click();
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
        for (QSlider *slider : qAsConst(sliders))
        {
            const bool isPreamp = (sliders.at(0) == slider);
            if (isPreamp)
                values[-1] = slider->value();
            else
                values[slider->property("idx").toInt()] = (getSliderCheckBox(slider)->isChecked() ? slider->value() : ~slider->value());
        }
        QByteArray dataArr;
        QDataStream stream(&dataArr, QIODevice::WriteOnly);
        stream << values;
        sets().set("Equalizer/Preset" + name, dataArr.toBase64().constData());

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
            for (QSlider *slider : qAsConst(sliders))
            {
                QCheckBox *checkB = getSliderCheckBox(slider);
                const bool isPreamp = (sliders.at(0) == slider);
                if (isPreamp)
                {
                    if (checkB->isChecked()) //Disable preamp auto for presets
                        checkB->click();
                    slider->setValue(values.value(-1));
                }
                else
                {
                    if (!checkB->isChecked()) //Enable slider
                        checkB->click();
                    const int value = values.value(slider->property("idx").toInt());
                    slider->setValue((value < 0) ? ~value : value);
                    if (value < 0) //Disable slider if necessary
                        checkB->click();
                }
            }
            if (!enabledB->isChecked())
                enabledB->click();
        }
    }
}

inline QCheckBox *EqualizerGUI::getSliderCheckBox(QSlider *slider)
{
    return (QCheckBox *)slider->property("checkbox").value<void *>();
}

void EqualizerGUI::sliderValueChanged(int idx, int v)
{
    const bool isAuto = getSliderCheckBox(sliders.at(0))->isChecked();
    bool canUpdateEqualizerNow = canUpdateEqualizer;

    if (!isAuto || idx >= 0)
        sets().set(QString("Equalizer/%1").arg(idx), v);

    if (idx > -1 && isAuto)
    {
        const int preampValue = sliders.at(0)->value();
        autoPreamp();
        if (canUpdateEqualizerNow && sliders.at(0)->value() != preampValue)
            canUpdateEqualizerNow = false; //Don't update equalizer instance now, because it will be updated from preamp slider.
    }

    setSliderInfo(idx, v);

    if (canUpdateEqualizerNow)
        setInstance<Equalizer>();
}
void EqualizerGUI::setSliderInfo(int idx, int v)
{
    QLabel *descrL = (QLabel *)sliders.at(idx + 1)->property("label").value<void *>();
    QString text = descrL->text();
    const int nIdx = text.indexOf('\n');
    text.remove(nIdx + 1, text.length() - nIdx + 1);
    text.append(Functions::dBStr(Equalizer::getAmpl((v < 0 && idx == -1) ? ~v : v)));
    descrL->setText(text);

    graph.setValue(idx, ((v < 0) ? (idx == -1 ? ~v : -1) : v) / 100.0f);
}

void EqualizerGUI::autoPreamp()
{
    int value = 0;
    for (int i = 1; i < sliders.count(); ++i)
    {
        QSlider *slider = sliders.at(i);
        value = qMax(getSliderCheckBox(slider)->isChecked() ? slider->value() : 0, value);
    }
    sliders.at(0)->setValue(100 - value);
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

void EqualizerGUI::showEvent(QShowEvent *event)
{
    autoCheckBoxSpacer->setMinimumHeight(getSliderCheckBox(sliders.at(0))->height());
    QWidget::showEvent(event);
}

QMap<int, int> EqualizerGUI::getPresetValues(const QString &name)
{
    QByteArray dataArr = QByteArray::fromBase64(sets().getByteArray("Equalizer/Preset" + name));
    QDataStream stream(&dataArr, QIODevice::ReadOnly);
    QMap<int, int> values;
    stream >> values;
    return values;
}
