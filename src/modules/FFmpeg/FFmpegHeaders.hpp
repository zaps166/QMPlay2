// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef Q_MOC_RUN

# include <QOperatingSystemVersion>
# include <QRegularExpression>
# include <QVarLengthArray>
# include <QVersionNumber>
# include <QWaitCondition>
# include <QLibrary>
# include <QProcess>
# include <QSysInfo>
# include <QThread>
# include <QDebug>

# include <QOpenGLFunctions>
# include <QGuiApplication>
# include <QOpenGLContext>
# include <QImage>

# include <QFormLayout>
# include <QGridLayout>
# include <QCheckBox>
# include <QComboBox>
# include <QGroupBox>
# include <QSpinBox>
# include <QLabel>

# include <limits>

extern "C" {
#  ifdef QMPlay2_VTB
#   include <libavutil/hwcontext_videotoolbox.h>
#   include <libavcodec/videotoolbox.h>
#  endif
#  ifdef QMPlay2_D3D11VA
#   include <libavutil/hwcontext_d3d11va.h>
#  endif
#  ifdef QMPlay2_VKVIDEO
#   include <libavutil/hwcontext_vulkan.h>
#  endif
#  ifdef QMPlay2_DXVA2
#   include <libavutil/hwcontext_dxva2.h>
#  endif
#  ifdef QMPlay2_VAAPI
#   include <libavutil/hwcontext_vaapi.h>
#  endif
#  ifdef QMPlay2_libavdevice
#   include <libavdevice/avdevice.h>
#  endif
#  include <libavformat/avformat.h>
#  include <libavutil/replaygain.h>
#  include <libavformat/version.h>
#  include <libavutil/hwcontext.h>
#  include <libavutil/spherical.h>
#  include <libavutil/display.h>
#  include <libavformat/avio.h>
#  include <libavutil/buffer.h>
}

# ifdef USE_VULKAN
#  include "../../qmvk/MemoryPropertyFlags.hpp"
#  include "../../qmvk/AbstractInstance.hpp"
#  include "../../qmvk/PhysicalDevice.hpp"
#  include "../../qmvk/CommandBuffer.hpp"
#  include "../../qmvk/BufferView.hpp"
#  include "../../qmvk/Buffer.hpp"
#  include "../../qmvk/Device.hpp"
#  include "../../qmvk/Image.hpp"
#  include "../../qmvk/Queue.hpp"
# endif

# ifdef QMPlay2_VAAPI
#  include <va/va.h>
#  include <va/va_vpp.h>
#  include <va/va_drm.h>
#  include <va/va_drmcommon.h>
#  ifdef USE_OPENGL
#   include <EGL/egl.h>
#   include <EGL/eglext.h>
#  endif
# endif

# include <fcntl.h>

# ifdef QMPlay2_D3D11VA
#  include <d3dcompiler.h>
#  include <wrl/client.h>
#  include <dxgi1_2.h>
#  include <d3d11.h>
# endif

# ifdef QMPlay2_DXVA2
#  include <d3d9.h>
#  include <dxva2api.h>
#  include <GL/wglext.h>
# endif

#endif
