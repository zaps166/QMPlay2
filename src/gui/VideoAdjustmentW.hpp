// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
