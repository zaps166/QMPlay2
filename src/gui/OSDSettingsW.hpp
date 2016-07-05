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
#include "ui_OSDSettings.h"

class QRadioButton;

class OSDSettingsW : public QWidget, public Ui::OSDSettings
{
public:
	static void init(const QString &, int, int, int, int, int, int, double, double, const QColor &, const QColor &, const QColor &);

	OSDSettingsW(const QString &);

	void writeSettings();
private:
	void readSettings();

	QString prefix;

	QRadioButton *alignB[9];

};

#endif //OSDSETTINGSW_HPP
