// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QStringList>

struct PaDeviceInfo;

namespace PortAudioCommon
{
    QString getOutputDeviceName(const PaDeviceInfo *deviceInfo);
    QStringList getOutputDeviceNames();
    int getDeviceIndexForOutput(const QString &name, const int chn = 0);
}
