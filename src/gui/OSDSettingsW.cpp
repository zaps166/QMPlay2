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

#include <OSDSettingsW.hpp>

#include <LibASS.hpp>
#include <Main.hpp>
#include <Settings.hpp>

#include <QRadioButton>

void OSDSettingsW::init(const QString &prefix, int a, int b, int c, int d, int e, int f, double g, double h, const QColor &i, const QColor &j, const QColor &k)
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.init(prefix + "/FontName", QFontComboBox().currentText());
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
	int align = QMPSettings.getInt(prefix + "/Alignment");
	if (align < 0 || align > 8)
		QMPSettings.set(prefix + "/Alignment", f);
}

OSDSettingsW::OSDSettingsW(const QString &prefix) :
	prefix(prefix)
{
	setupUi(this);

	// Alignment GroupBox
	QGridLayout* alignL = new QGridLayout(alignGB);
	for (int align = 0; align < 9; align++)
	{
		alignB[align] = new QRadioButton;
		alignL->addWidget(alignB[align], align/3, align%3);
	}

	readSettings();

	setDisabled(LibASS::isDummy());
}

void OSDSettingsW::writeSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.set(prefix + "/FontName", fontCB->currentText());
	QMPSettings.set(prefix + "/FontSize", fontSizeB->value());
	QMPSettings.set(prefix + "/Spacing", spacingB->value());
	QMPSettings.set(prefix + "/LeftMargin", leftMarginB->value());
	QMPSettings.set(prefix + "/RightMargin", rightMarginB->value());
	QMPSettings.set(prefix + "/VMargin", vMarginB->value());
	for (int align = 0; align < 9; ++align)
		if (alignB[align]->isChecked())
		{
			QMPSettings.set(prefix + "/Alignment", align);
			break;
		}
	QMPSettings.set(prefix + "/Outline", outlineB->value());
	QMPSettings.set(prefix + "/Shadow", shadowB->value());
	QMPSettings.set(prefix + "/TextColor", textColorB->getColor());
	QMPSettings.set(prefix + "/OutlineColor", outlineColorB->getColor());
	QMPSettings.set(prefix + "/ShadowColor", shadowColorB->getColor());
}

void OSDSettingsW::readSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	int idx = fontCB->findText(QMPSettings.getString(prefix + "/FontName"));
	if (idx > -1)
		fontCB->setCurrentIndex(idx);
	fontSizeB->setValue(QMPSettings.getInt(prefix + "/FontSize"));
	spacingB->setValue(QMPSettings.getInt(prefix + "/Spacing"));
	leftMarginB->setValue(QMPSettings.getInt(prefix + "/LeftMargin"));
	rightMarginB->setValue(QMPSettings.getInt(prefix + "/RightMargin"));
	vMarginB->setValue(QMPSettings.getInt(prefix + "/VMargin"));
	int align = QMPSettings.getInt(prefix + "/Alignment");
	alignB[align]->setChecked(true);
	outlineB->setValue(QMPSettings.getDouble(prefix + "/Outline"));
	shadowB->setValue(QMPSettings.getDouble(prefix + "/Shadow"));
	textColorB->setColor(QMPSettings.get(prefix + "/TextColor").value<QColor>());
	outlineColorB->setColor(QMPSettings.get(prefix + "/OutlineColor").value<QColor>());
	shadowColorB->setColor(QMPSettings.get(prefix + "/ShadowColor").value<QColor>());
}
