/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#ifndef LINEEDIT_HPP
#define LINEEDIT_HPP

#include <QLineEdit>
#include <QLabel>

class LineEditButton : public QLabel
{
	Q_OBJECT
public:
	LineEditButton();
private:
	void mousePressEvent(QMouseEvent *) override final;
signals:
	void clicked();
};

/**/

class LineEdit : public QLineEdit
{
	Q_OBJECT
public:
	LineEdit(QWidget *parent = nullptr);
private:
	void resizeEvent(QResizeEvent *) override final;
	void mousePressEvent(QMouseEvent *) override final;
	void mouseMoveEvent(QMouseEvent *) override final;

	LineEditButton b;
private slots:
	void textChangedSlot(const QString &);
public slots:
	void clearText();
signals:
	void clearButtonClicked();
};

#endif
