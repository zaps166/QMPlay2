// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ALSAHeaders.hpp>

namespace ALSACommon
{
    using DevicesList = QPair<QStringList, QStringList>;

    DevicesList getDevices();

    QString getDeviceName(const DevicesList &devicesList, const QString &deviceName);
}
