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
    Module("AudioCD"),
    CD(":/CD.svgz"),
    cdioDestroyTimer(new CDIODestroyTimer)
{
    m_icon = QIcon(":/AudioCD.svgz");

    init("AudioCD/CDDB", true);
    init("AudioCD/CDTEXT", true);
}
AudioCD::~AudioCD()
{
    delete cdioDestroyTimer;
    libcddb_shutdown();
}

QList<AudioCD::Info> AudioCD::getModulesInfo(const bool) const
{
    QList<Info> modulesInfo;
#ifdef Q_OS_WIN
    modulesInfo += Info(AudioCDName, DEMUXER, QStringList{"cda"}, CD);
#else
    modulesInfo += Info(AudioCDName, DEMUXER, CD);
#endif
    return modulesInfo;
}
void *AudioCD::createInstance(const QString &name)
{
    if (name == AudioCDName)
        return new AudioCDDemux(*this, *cdioDestroyTimer);
    return nullptr;
}

QList<QAction *> AudioCD::getAddActions()
{
    QAction *actCD = new QAction(nullptr);
    actCD->setIcon(CD);
    actCD->setText(tr("AudioCD"));
    actCD->connect(actCD, SIGNAL(triggered()), this, SLOT(add()));
    return QList<QAction *>() << actCD;
}

AudioCD::SettingsWidget *AudioCD::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

void AudioCD::add()
{
    QWidget *parent = qobject_cast<QWidget *>(sender()->parent());
    QStringList drives = AudioCDDemux::getDevices();
    if (!drives.isEmpty())
    {
        QDialog chooseCD(parent);
        chooseCD.setWindowIcon(m_icon);
        chooseCD.setWindowTitle(tr("Choose the drive"));
        QLabel drvL(tr("Path") + ":");
        drvL.setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
        QComboBox drvB;
        QLineEdit drvE;
        connect(&drvB, SIGNAL(currentIndexChanged(const QString &)), &drvE, SLOT(setText(const QString &)));
        drvB.addItems(drives);
        QToolButton browseB;
        connect(&browseB, SIGNAL(clicked()), this, SLOT(browseCDImage()));
        browseB.setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
        QDialogButtonBox bb(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
        connect(&bb, SIGNAL(accepted()), &chooseCD, SLOT(accept()));
        connect(&bb, SIGNAL(rejected()), &chooseCD, SLOT(reject()));
        QGridLayout layout(&chooseCD);
        layout.addWidget(&drvB, 0, 0, 1, 3);
        layout.addWidget(&drvL, 1, 0, 1, 1);
        layout.addWidget(&drvE, 1, 1, 1, 1);
        layout.addWidget(&browseB, 1, 2, 1, 1);
        layout.addWidget(&bb, 2, 0, 1, 3);
        layout.setMargin(2);
        chooseCD.resize(400, 0);
        if (chooseCD.exec() == QDialog::Accepted)
        {
            QString drvPth = drvE.text();
#ifdef Q_OS_WIN
            if (drvPth.length() == 2 && !drvPth.endsWith("/"))
                drvPth += "/";
#endif
            emit QMPlay2Core.processParam("open", AudioCDName "://" + drvPth);
        }
    }
    else
        QMessageBox::information(parent, AudioCDName, tr("No CD/DVD drives found!"));
}
void AudioCD::browseCDImage()
{
    QWidget *parent = (QWidget *)sender()->parent();
    QString path = QFileDialog::getOpenFileName(parent, tr("Choose AudioCD image"), QString(), tr("Supported AudioCD images") + " (*.cue *.nrg *.toc)");
    if (!path.isEmpty())
    {
        QComboBox &drvB = *parent->findChild<QComboBox *>();
        drvB.addItem(path);
        drvB.setCurrentIndex(drvB.count() - 1);
    }
}

QMPLAY2_EXPORT_MODULE(AudioCD)

/**/

#include <QRadioButton>
#include <QGroupBox>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    audioCDB = new QGroupBox(tr("AudioCD"));

    useCDDB = new QCheckBox(tr("Use CDDB if CD-TEXT is not available"));
    useCDDB->setChecked(sets().getBool("AudioCD/CDDB"));

    useCDTEXT = new QCheckBox(tr("Use CD-TEXT"));
    useCDTEXT->setChecked(sets().getBool("AudioCD/CDTEXT"));

    QVBoxLayout *audioCDBLayout = new QVBoxLayout(audioCDB);
    audioCDBLayout->addWidget(useCDDB);
    audioCDBLayout->addWidget(useCDTEXT);

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(audioCDB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("AudioCD/CDDB", useCDDB->isChecked());
    sets().set("AudioCD/CDTEXT", useCDTEXT->isChecked());
}
