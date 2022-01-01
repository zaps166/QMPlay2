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

#pragma once

#include <Module.hpp>

class Inputs final : public Module
{
    Q_OBJECT
public:
    Inputs();
private:
    QList<Info> getModulesInfo(const bool) const override;
    void *createInstance(const QString &) override;

    QList<QAction *> getAddActions() override;

    SettingsWidget *getSettingsWidget() override;

    QIcon toneIcon, pcmIcon, rayman2Icon;
private slots:
    void add();
};

/**/

#include <QSpinBox>
#include <QDialog>

class HzW final : public QWidget
{
public:
    HzW(int, const QStringList &);

    QString getFreqs() const;
    inline void connectFreqs(const QObject *receiver, const char *method)
    {
        for (int i = 0; i < hzB.count(); ++i)
            hzB[i]->connect(hzB[i], SIGNAL(valueChanged(int)), receiver, method);
    }
private:
    QList<QSpinBox *> hzB;
};

class QGridLayout;

class AddD final : public QDialog
{
    Q_OBJECT
public:
    AddD(Settings &, QWidget *parent = nullptr, QObject *moduleSetsW = nullptr);

    inline int getSampleRate() const
    {
        return srateB->value();
    }
    inline QString getFreqs() const
    {
        return hzW->getFreqs();
    }

    void save();

    QString execAndGet();
private slots:
    void channelsChanged(int);
    void add();
private:
    QObject *moduleSetsW;
    QGridLayout *layout;
    QSpinBox *srateB;
    Settings &sets;
    HzW *hzW;
};

class QRadioButton;
class QComboBox;
class QGroupBox;
class QCheckBox;
class QLineEdit;

class ModuleSettingsWidget final : public Module::SettingsWidget
{
    Q_OBJECT
public:
    ModuleSettingsWidget(Module &);
private slots:
    void applyFreqs();
private:
    void saveSettings() override;

    AddD *toneGenerator;
    QGroupBox *pcmB;
    QLineEdit *pcmExtsE;
    QList<QRadioButton *> formatB;
    QSpinBox *chnB, *srateB, *offsetB;
    QComboBox *endianB;

    QCheckBox *rayman2EB;
};
