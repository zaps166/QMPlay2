/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <QWidget>

#include <vector>
#include <array>

class ModuleParams;
class QAction;
class Slider;

class VideoAdjustmentW final : public QWidget
{
    Q_OBJECT

public:
    VideoAdjustmentW();
    ~VideoAdjustmentW();

    void restoreValues();
    void saveValues();

    void setModuleParam(ModuleParams *writer);
    void enableControls();

    void setKeyShortcuts();
    void addActionsToWidget(QWidget *w);

signals:
    void videoAdjustmentChanged(const QString &osdText);

private:
    std::vector<Slider *> m_sliders;
    std::vector<std::array<QAction *, 2>> m_actions;
    QAction *m_resetAction = nullptr;
};
