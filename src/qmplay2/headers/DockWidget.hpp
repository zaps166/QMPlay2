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

#pragma once

#include <QDockWidget>

class DockWidget : public QDockWidget
{
public:
	inline DockWidget() :
		titleBarVisible(true), globalTitleBarVisible(true)
	{}
	inline void setTitleBarVisible(bool v = true)
	{
		setTitleBarWidget((titleBarVisible = v) && globalTitleBarVisible ? nullptr : &emptyW);
	}
	inline void setGlobalTitleBarVisible(bool v)
	{
		globalTitleBarVisible = v;
		setTitleBarVisible(titleBarVisible);
	}
private:
	class EmptyW : public QWidget
	{
		QSize sizeHint() const override final;
	} emptyW;
	bool titleBarVisible, globalTitleBarVisible;
};
