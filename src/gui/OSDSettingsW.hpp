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

#ifndef OSDSETTINGSW_HPP
#define OSDSETTINGSW_HPP

#include <QWidget>

class QDoubleSpinBox;
class QFontComboBox;
class QRadioButton;
class ColorButton;
class QGridLayout;
class QGroupBox;
class QSpinBox;
class QLabel;

class OSDSettingsW : public QWidget
{
	Q_OBJECT
public:
	static void init(const QString &, int, int, int, int, int, int, double, double, const QColor &, const QColor &, const QColor &);

	OSDSettingsW(const QString &);

	void writeSettings();

	QGridLayout *layout, *fontL, *marginsL, *alignL, *frameL, *colorsL;
private:
	void readSettings();

	QString prefix;

	QGroupBox *fontGB, *marginsGB, *alignGB, *frameGB, *colorsGB;

	QLabel *fontSizeL, *spacingL, *leftMarginL, *rightMarginL, *vMarginL, *outlineL, *shadowL, *textColorL, *outlineColorL, *shadowColorL;
	QFontComboBox *fontCB;
	QSpinBox *fontSizeB, *spacingB, *leftMarginB, *rightMarginB, *vMarginB;
	QDoubleSpinBox *outlineB, *shadowB;
	ColorButton *textColorB, *outlineColorB, *shadowColorB;
	QRadioButton *alignB[9];

};

#endif //OSDSETTINGSW_HPP
