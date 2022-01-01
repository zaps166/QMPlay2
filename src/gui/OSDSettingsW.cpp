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

#include <OSDSettingsW.hpp>

#include <Settings.hpp>
#include <LibASS.hpp>

#include <QRadioButton>

#include "ui_OSDSettings.h"

static void appendColon(const QObjectList &objects)
{
    for (QObject *obj : objects)
    {
        appendColon(obj->children());
        if (QLabel *label = qobject_cast<QLabel *>(obj))
            label->setText(label->text() + ": ");
    }
}

void OSDSettingsW::init(const QString &prefix, int a, int b, int c, int d, int e, int f, double g, double h, const QColor &i, const QColor &j, const QColor &k)
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    QMPSettings.init(prefix + "/FontName", QGuiApplication::font().family());
    QMPSettings.init(prefix + "/FontSize", a);
    QMPSettings.init(prefix + "/Spacing", b);
    QMPSettings.init(prefix + "/LeftMargin", c);
    QMPSettings.init(prefix + "/RightMargin", d);
    QMPSettings.init(prefix + "/VMargin", e);
    QMPSettings.init(prefix + "/Alignment", f);
    QMPSettings.init(prefix + "/Outline", g);
    QMPSettings.init(prefix + "/Shadow", h);
    QMPSettings.init(prefix + "/TextColor", i);
    QMPSettings.init(prefix + "/OutlineColor", j);
    QMPSettings.init(prefix + "/ShadowColor", k);
    const int align = QMPSettings.getInt(prefix + "/Alignment");
    if (align < 0 || align > 8)
        QMPSettings.set(prefix + "/Alignment", f);
}

OSDSettingsW::OSDSettingsW(const QString &prefix) :
    ui(new Ui::OSDSettings),
    prefix(prefix)
{
    ui->setupUi(this);
    appendColon(children());

    // Alignment GroupBox
    QGridLayout *alignL = new QGridLayout(ui->alignGB);
    for (int align = 0; align < 9; ++align)
    {
        alignB[align] = new QRadioButton;
        alignL->addWidget(alignB[align], align / 3, align % 3);
    }

    readSettings();

    setDisabled(LibASS::isDummy());
}
OSDSettingsW::~OSDSettingsW()
{
    delete ui;
}

void OSDSettingsW::addWidget(QWidget *w)
{
    ui->layout->addWidget(w);
}

void OSDSettingsW::writeSettings()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    QMPSettings.set(prefix + "/FontName", ui->fontCB->currentText());
    QMPSettings.set(prefix + "/FontSize", ui->fontSizeB->value());
    QMPSettings.set(prefix + "/Spacing", ui->spacingB->value());
    QMPSettings.set(prefix + "/LeftMargin", ui->leftMarginB->value());
    QMPSettings.set(prefix + "/RightMargin", ui->rightMarginB->value());
    QMPSettings.set(prefix + "/VMargin", ui->vMarginB->value());
    for (int align = 0; align < 9; ++align)
        if (alignB[align]->isChecked())
        {
            QMPSettings.set(prefix + "/Alignment", align);
            break;
        }
    QMPSettings.set(prefix + "/Outline", ui->outlineB->value());
    QMPSettings.set(prefix + "/Shadow", ui->shadowB->value());
    QMPSettings.set(prefix + "/TextColor", ui->textColorB->getColor());
    QMPSettings.set(prefix + "/OutlineColor", ui->outlineColorB->getColor());
    QMPSettings.set(prefix + "/ShadowColor", ui->shadowColorB->getColor());
}

void OSDSettingsW::readSettings()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    const int idx = ui->fontCB->findText(QMPSettings.getString(prefix + "/FontName"));
    if (idx > -1)
        ui->fontCB->setCurrentIndex(idx);
    ui->fontSizeB->setValue(QMPSettings.getInt(prefix + "/FontSize"));
    ui->spacingB->setValue(QMPSettings.getInt(prefix + "/Spacing"));
    ui->leftMarginB->setValue(QMPSettings.getInt(prefix + "/LeftMargin"));
    ui->rightMarginB->setValue(QMPSettings.getInt(prefix + "/RightMargin"));
    ui->vMarginB->setValue(QMPSettings.getInt(prefix + "/VMargin"));
    alignB[QMPSettings.getInt(prefix + "/Alignment")]->setChecked(true);
    ui->outlineB->setValue(QMPSettings.getDouble(prefix + "/Outline"));
    ui->shadowB->setValue(QMPSettings.getDouble(prefix + "/Shadow"));
    ui->textColorB->setColor(QMPSettings.getColor(prefix + "/TextColor"));
    ui->outlineColorB->setColor(QMPSettings.getColor(prefix + "/OutlineColor"));
    ui->shadowColorB->setColor(QMPSettings.getColor(prefix + "/ShadowColor"));
}
