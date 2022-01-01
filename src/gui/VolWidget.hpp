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

#include <Slider.hpp>

#include <QWidget>
#include <QMenu>

class VolWidget : public QWidget
{
    Q_OBJECT
public:
    VolWidget(int max);

    inline int volumeL() const
    {
        return vol[0].value();
    }
    inline int volumeR() const
    {
        return vol[1].value();
    }

    void setVolume(int volL, int volR, bool first = false);
    void changeVolume(int deltaVol);
private slots:
    void customContextMenuRequested(const QPoint &pos);
    void splitTriggered(bool splitted);
    void setMaximumVolume(int max);

    void sliderValueChanged(int v);
signals:
    void volumeChanged(int l, int r);
private:
    void setSlidersToolTip();

    QAction *splitA;
    Slider vol[2];
    QMenu menu;
};
