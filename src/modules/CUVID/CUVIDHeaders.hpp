// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef Q_MOC_RUN

# include <QOperatingSystemVersion>
# include <QLibrary>
# include <QDebug>

# include <QOpenGLContext>
# include <QImage>

# include <QGridLayout>
# include <QCheckBox>
# include <QComboBox>
# include <QLabel>

extern "C" {
#  include <libavcodec/bsf.h>
}

# ifdef USE_VULKAN
#  include "../../qmvk/AbstractInstance.hpp"
#  include "../../qmvk/PhysicalDevice.hpp"
#  include "../../qmvk/Semaphore.hpp"
#  include "../../qmvk/Device.hpp"
#  include "../../qmvk/Image.hpp"
# endif

# include <cuda/CUDAQMPlayTypes.hpp>
# include <cuda/CUVIDQMPlayTypes.hpp>

#endif
