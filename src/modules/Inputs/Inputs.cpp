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

#include <Inputs.hpp>
#include <ToneGenerator.hpp>
#include <PCM.hpp>
#include <Rayman2.hpp>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialog>
#include <QAction>
#include <QLabel>

constexpr const char *g_standartExts = "pcm;raw";
constexpr const char *g_formatName[PCM::FORMAT_COUNT] = {
    "Unsigned 8bit PCM",
    "Signed 8bit PCM",
    "Signed 16bit PCM",
    "Signed 24bit PCM",
    "Signed 32bit PCM",
    "Float 32bit"
};

/**/

Inputs::Inputs() :
    Module("Inputs"),
    toneIcon(":/ToneGenerator.svgz"), pcmIcon(":/PCM.svgz"), rayman2Icon(":/Rayman2")
{
    m_icon = QIcon(":/Inputs.svgz");

    init("ToneGenerator/srate", 48000);
    init("ToneGenerator/freqs", 440);
    init("PCM", true);
    if (getStringList("PCM/extensions").isEmpty())
        set("PCM/extensions", QString(g_standartExts).split(';'));
    if (getUInt("PCM/format") >= PCM::FORMAT_COUNT)
        set("PCM/format", 2);
    init("PCM/chn", 2);
    init("PCM/srate", 44100);
    init("PCM/offset", 0);
    init("PCM/BE", false);
    init("Rayman2", true);
}

QList<Inputs::Info> Inputs::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    modulesInfo += Info(ToneGeneratorName, DEMUXER, toneIcon);
    if (showDisabled || getBool("PCM"))
        modulesInfo += Info(PCMName, DEMUXER, getStringList("PCM/extensions"), pcmIcon);
    if (showDisabled || getBool("Rayman2"))
        modulesInfo += Info(Rayman2Name, DEMUXER, QStringList{"apm"}, rayman2Icon);
    return modulesInfo;
}
void *Inputs::createInstance(const QString &name)
{
    if (name == ToneGeneratorName)
        return new ToneGenerator(*this);
    else if (name == PCMName)
        return new PCM(*this);
    else if (name == Rayman2Name)
        return new Rayman2(*this);
    return nullptr;
}

QList<QAction *> Inputs::getAddActions()
{
    QAction *actTone = new QAction(nullptr);
    actTone->setIcon(toneIcon);
    actTone->setText(tr("Tone generator"));
    actTone->connect(actTone, SIGNAL(triggered()), this, SLOT(add()));
    return QList<QAction *>() << actTone;
}

Inputs::SettingsWidget *Inputs::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

void Inputs::add()
{
    QWidget *parent = qobject_cast<QWidget *>(sender()->parent());
    AddD d(*this, parent);
    d.setWindowIcon(toneIcon);
    const QString params = d.execAndGet();
    if (!params.isEmpty())
        emit QMPlay2Core.processParam("open", ToneGeneratorName "://" + params);
}

QMPLAY2_EXPORT_MODULE(Inputs)

/**/

#include <QPushButton>
#include <QGroupBox>

HzW::HzW(int c, const QStringList &freqs)
{
    QGridLayout *layout = new QGridLayout(this);
    for (int i = 0; i < c; ++i)
    {
        QSpinBox *sB = new QSpinBox;
        sB->setRange(0, 96000);
        sB->setSuffix(" Hz");
        if (freqs.count() > i)
            sB->setValue(freqs[i].toInt());
        else
            sB->setValue(440);
        hzB.append(sB);
        layout->addWidget(sB, i/4, i%4);
    }
}

QString HzW::getFreqs() const
{
    QString freqs;
    for (QSpinBox *sB : hzB)
        freqs += QString::number(sB->value()) + ",";
    freqs.chop(1);
    return freqs;
}

/**/

AddD::AddD(Settings &sets, QWidget *parent, QObject *moduleSetsW) :
    QDialog(parent), moduleSetsW(moduleSetsW), sets(sets), hzW(nullptr)
{
    QGroupBox *gB = nullptr;
    if (parent)
        setWindowTitle(tr("Tone generator"));
    else
        gB = new QGroupBox(tr("Tone generator"));

    QLabel *channelsL = new QLabel(tr("Channel count") + ": ");

    QSpinBox *channelsB = new QSpinBox;
    connect(channelsB, SIGNAL(valueChanged(int)), this, SLOT(channelsChanged(int)));

    QLabel *srateL = new QLabel(tr("Sample rate") + ": ");

    srateB = new QSpinBox;
    srateB->setRange(4, 384000);
    srateB->setSuffix(" Hz");
    srateB->setValue(sets.getInt("ToneGenerator/srate"));

    QDialogButtonBox *bb = nullptr;
    QPushButton *addB = nullptr;
    if (parent)
    {
        bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
        connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
        connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
    }
    else
    {
        addB = new QPushButton(tr("Play"));
        addB->setIcon(QIcon(":/sine"));
        connect(addB, SIGNAL(clicked()), this, SLOT(add()));
    }

    layout = new QGridLayout(parent ? (QWidget *)this : (QWidget *)gB);
    layout->addWidget(channelsL, 0, 0, 1, 1);
    layout->addWidget(channelsB, 0, 1, 1, 1);
    layout->addWidget(srateL, 1, 0, 1, 1);
    layout->addWidget(srateB, 1, 1, 1, 1);
    if (parent)
        layout->addWidget(bb, 3, 0, 1, 2);
    else
    {
        layout->addWidget(addB, 3, 0, 1, 2);
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);
        layout->addWidget(gB);
    }

    channelsB->setRange(1, 8);
    channelsB->setValue(sets.getString("ToneGenerator/freqs").split(',').count());
}

void AddD::save()
{
    sets.set("ToneGenerator/srate", getSampleRate());
    sets.set("ToneGenerator/freqs", getFreqs());
}

QString AddD::execAndGet()
{
    if (exec() == QDialog::Accepted)
        return "{samplerate=" + QString::number(getSampleRate()) + "&freqs=" + getFreqs() + "}";
    return QString();
}

void AddD::channelsChanged(int c)
{
    delete hzW;
    hzW = new HzW(c, sets.getString("ToneGenerator/freqs").split(','));
    layout->addWidget(hzW, 2, 0, 1, 2);
    if (moduleSetsW)
        hzW->connectFreqs(moduleSetsW, SLOT(applyFreqs()));
}
void AddD::add()
{
    save();
    emit QMPlay2Core.processParam("open", ToneGeneratorName "://{}");
}

/**/

#include <QRadioButton>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    toneGenerator = new AddD(sets(), nullptr, this);

    pcmB = new QGroupBox(tr("Uncompressed PCM sound"));
    pcmB->setCheckable(true);
    pcmB->setChecked(sets().getBool("PCM"));

    QLabel *pcmExtsL = new QLabel(tr("File extensions (semicolon separated)") + ": ");

    pcmExtsE = new QLineEdit;
    QString exts;
    for (const QString &ext : sets().getStringList("PCM/extensions"))
        exts += ext + ";";
    pcmExtsE->setText(exts);

    QGroupBox *fmtB = new QGroupBox(tr("Format"));
    QVBoxLayout *fmtLayout = new QVBoxLayout(fmtB);
    int checked = sets().getInt("PCM/format");
    for (int i = 0; i < PCM::FORMAT_COUNT; ++i)
    {
        QRadioButton *rB = new QRadioButton(g_formatName[i]);
        if (i == checked)
            rB->setChecked(true);
        fmtLayout->addWidget(rB);
        formatB.append(rB);
    }

    QLabel *chnL = new QLabel(tr("Channel count") + ": ");

    chnB = new QSpinBox;
    chnB->setRange(1, 8);
    chnB->setValue(sets().getInt("PCM/chn"));

    QLabel *srateL = new QLabel(tr("Sample rate") + ": ");

    srateB = new QSpinBox;
    srateB->setSuffix(" Hz");
    srateB->setRange(2, 1000000000);
    srateB->setValue(sets().getInt("PCM/srate"));

    QLabel *offsetL = new QLabel(tr("Offset") + ": ");

    offsetB = new QSpinBox;
    offsetB->setSuffix(" B");
    offsetB->setRange(0, 0x7FFFFFFF);
    offsetB->setValue(sets().getInt("PCM/offset"));

    endianB = new QComboBox;
    endianB->addItem("Little endian");
    endianB->addItem("Big endian");
    endianB->setCurrentIndex(sets().getBool("PCM/BE"));

    QGridLayout *pcmLayout = new QGridLayout(pcmB);
    pcmLayout->addWidget(pcmExtsL, 0, 0, 1, 1);
    pcmLayout->addWidget(pcmExtsE, 0, 1, 1, 2);
    pcmLayout->addWidget(fmtB, 1, 0, 5, 1);
    pcmLayout->addWidget(chnL, 1, 1, 1, 1);
    pcmLayout->addWidget(chnB, 1, 2, 1, 1);
    pcmLayout->addWidget(srateL, 2, 1, 1, 1);
    pcmLayout->addWidget(srateB, 2, 2, 1, 1);
    pcmLayout->addWidget(offsetL, 3, 1, 1, 1);
    pcmLayout->addWidget(offsetB, 3, 2, 1, 1);
    pcmLayout->addWidget(endianB, 4, 1, 1, 2);
    pcmLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 5, 1, 1, 2);

    rayman2EB = new QCheckBox(tr("Rayman2 music (*.apm)"));
    rayman2EB->setChecked(sets().getBool("Rayman2"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(toneGenerator);
    layout->addWidget(pcmB);
    layout->addWidget(rayman2EB);
}

void ModuleSettingsWidget::applyFreqs()
{
    toneGenerator->save();
    SetInstance<ToneGenerator>();
}

void ModuleSettingsWidget::saveSettings()
{
    toneGenerator->save();
    if (pcmExtsE->text().isEmpty())
        pcmExtsE->setText(g_standartExts);
    sets().set("PCM", pcmB->isChecked());
    sets().set("PCM/extensions", pcmExtsE->text().split(';', QString::SkipEmptyParts));
    for (int i = 0; i < formatB.size(); ++i)
        if (formatB[i]->isChecked())
        {
            sets().set("PCM/format", i);
            break;
        }
    sets().set("PCM/chn", chnB->value());
    sets().set("PCM/srate", srateB->value());
    sets().set("PCM/offset", offsetB->value());
    sets().set("PCM/BE", (bool)endianB->currentIndex());
    sets().set("Rayman2", rayman2EB->isChecked());
}
