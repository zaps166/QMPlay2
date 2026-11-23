// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef Q_MOC_RUN

# include <QStringList>
# include <QThread>
# include <QDebug>

# include <QDoubleSpinBox>
# include <QFormLayout>
# include <QPushButton>
# include <QCheckBox>
# include <QComboBox>
# include <QLabel>

# include <portaudio.h>

# ifdef Q_OS_WIN
#  include <mmdeviceapi.h>
#  include <pa_win_wasapi.h>
#  include <audioclient.h>
# endif

# ifdef Q_OS_MACOS
#  include "3rdparty/CoreAudio/AudioDeviceList.h"
#  include "3rdparty/CoreAudio/AudioDevice.h"
# endif

#endif
