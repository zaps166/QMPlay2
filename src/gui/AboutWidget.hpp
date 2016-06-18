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

#ifndef ABOUTWIDGET_HPP
#define ABOUTWIDGET_HPP

#include <QWidget>
#include <QTextEdit>
#include <QFileSystemWatcher>

class CommunityEdit : public QTextEdit
{
	Q_OBJECT
private:
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *);
};

/**/

class QPushButton;

class AboutWidget : public QWidget
{
	Q_OBJECT
public:
	AboutWidget();
private:
	void showEvent(QShowEvent *);
	void closeEvent(QCloseEvent *);

	CommunityEdit *dE;
	QTextEdit *logE, *clE;
	QPushButton *clrLogB;
	QFileSystemWatcher logWatcher;
private slots:
	void refreshLog();
	void clrLog();
	void currentTabChanged(int);
};

#endif //ABOUTWIDGET_HPP
