/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <QMPlay2Lib.hpp>

#include <QPushButton>

class QMPLAY2SHAREDLIB_EXPORT ColorButton final : public QPushButton
{
	Q_OBJECT
public:
	ColorButton(QWidget *parent = nullptr);

	void setColor(const QColor &color);
	inline QColor getColor() const
	{
		return m_color;
	}
protected:
	void paintEvent(QPaintEvent *) override;
private:
	QColor m_color;
	bool m_alphaChannel;
private slots:
	void openColorDialog();
signals:
	void colorChanged();
};
