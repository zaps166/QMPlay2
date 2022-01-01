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

#include <functional>

class QString;
class QWindow;

namespace QMPlay2MacExtensions
{
    using MediaKeysCallback = std::function<void(const QString &param)>;

    void setApplicationVisible(bool visible);

    void registerMacOSMediaKeys(const MediaKeysCallback &fn);
    void unregisterMacOSMediaKeys();

    void showSystemUi(QWindow *mainWindow, bool visible);
}
