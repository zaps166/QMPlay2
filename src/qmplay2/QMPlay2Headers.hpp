// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#ifndef Q_MOC_RUN // Global includes

# include <QCoreApplication>
# include <QElapsedTimer>
# include <QByteArray>
# include <QFileInfo>
# include <QSettings>
# include <QPointer>
# include <QVariant>
# include <QObject>
# include <QString>
# include <QVector>
# include <QMutex>
# include <QQueue>
# include <QFile>
# include <QHash>
# include <QList>
# include <QPair>
# include <QRect>
# include <QDir>
# include <QMap>
# include <QSet>

# ifdef USE_VULKAN
#  include <QVulkanInstance>
# endif
# include <QWheelEvent>
# include <QVector4D>
# include <QColor>
# include <QIcon>

# include <QSystemTrayIcon>

# include <unordered_map>
# include <unordered_set>
# include <functional>
# include <algorithm>
# include <optional>
# include <atomic>
# include <memory>
# include <vector>
# include <array>
# include <deque>
# include <mutex>
# include <any>
# include <set>

extern "C" {
#  include <libavutil/mastering_display_metadata.h>
#  include <libswresample/swresample.h>
#  include <libavcodec/avcodec.h>
#  include <libavutil/rational.h>
#  include <libswscale/swscale.h>
#  include <libavutil/pixdesc.h>
#  include <libavutil/version.h>
#  include <libavutil/pixfmt.h>
#  include <libavutil/cpu.h>
#  include <libavutil/mem.h>
#  if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
#   include <libavutil/tx.h>
#  else
#   include <libavcodec/avfft.h>
#  endif
}

# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>
# include <math.h>

# include <time.h>

# ifndef Q_OS_WIN
#  include <unistd.h>
# endif

# ifdef Q_OS_WIN
#  ifdef WINVER
#   undef WINVER
#  endif
#  define WINVER 0x0601
#  include <windows.h>
# endif

# ifdef Q_OS_MACOS
#  include <mach/mach_time.h>
# endif

#endif

#if !defined(Q_MOC_RUN) && defined(QMPLAY2SHAREDLIB_LIBRARY) // Local includes

# include <QMessageAuthenticationCode>
# include <QOperatingSystemVersion>
# include <QRegularExpression>
# include <QVariantAnimation>
# include <QLoggingCategory>
# include <QSocketNotifier>
# include <QStandardPaths>
# include <QWaitCondition>
# include <QJsonDocument>
# include <QLibraryInfo>
# include <QStringList>
# include <QTranslator>
# include <QJsonArray>
# include <QTextCodec>
# include <QDateTime>
# include <QIODevice>
# include <QMimeData>
# include <QResource>
# include <QSaveFile>
# include <QLibrary>
# include <QProcess>
# include <QBuffer>
# include <QLocale>
# include <QThread>
# include <QTimer>
# include <QDate>

# ifdef USE_OPENGL
#  include <QOpenGLExtraFunctions>
#  include <QOpenGLShaderProgram>
#  include <QOffscreenSurface>
#  include <QOpenGLFunctions>
#  include <QOpenGLShader>
#  include <QOpenGLWindow>
#  include <QOpenGLWidget>
#  if !defined(QT_OPENGL_ES_2) && !defined(QT_FEATURE_opengles2)
#   include <QOpenGLFunctions_1_5>
#  endif
# endif

# include <QTextDocumentFragment>
# include <QMatrix4x4>
# include <QPainter>
# include <QAction>
# include <QWindow>

# include <QGraphicsDropShadowEffect>
# include <QGraphicsBlurEffect>
# include <QGraphicsPixmapItem>
# include <QStyleOptionSlider>
# include <QGraphicsScene>
# include <QApplication>
# include <QColorDialog>
# include <QInputDialog>
# include <QStyleOption>
# include <QDockWidget>
# include <QHeaderView>
# include <QMessageBox>
# include <QPushButton>
# include <QTreeWidget>
# include <QLineEdit>

# if defined(DBUS_PM) || defined(NOTIFIES_FREEDESKTOP)
#  include <QDBusPendingCallWatcher>
#  include <QDBusConnection>
#  include <QDBusInterface>
# endif

# ifdef USE_QML
#  include <QJSEngine>
#  include <QJSValue>
# endif

extern "C" {
#  include <libavformat/avformat.h>
#  include <libavutil/imgutils.h>
#  include <libavutil/avutil.h>
#  if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 91, 100)
#   include <libavcodec/bsf.h>
#  endif
#  include <libavutil/opt.h>
}

# ifdef QMPLAY2_RUBBERBAND
#  include <rubberband/RubberBandStretcher.h>
# endif

# ifdef USE_VULKAN
#  include "../qmvk/DescriptorSetLayout.hpp"
#  include "../qmvk/MemoryPropertyFlags.hpp"
#  include "../qmvk/MemoryObjectDescr.hpp"
#  include "../qmvk/AbstractInstance.hpp"
#  include "../qmvk/GraphicsPipeline.hpp"
#  include "../qmvk/ComputePipeline.hpp"
#  include "../qmvk/DescriptorPool.hpp"
#  include "../qmvk/PhysicalDevice.hpp"
#  include "../qmvk/CommandBuffer.hpp"
#  include "../qmvk/ShaderModule.hpp"
#  include "../qmvk/BufferView.hpp"
#  include "../qmvk/RenderPass.hpp"
#  include "../qmvk/SwapChain.hpp"
#  include "../qmvk/Sampler.hpp"
#  include "../qmvk/Buffer.hpp"
#  include "../qmvk/Device.hpp"
#  include "../qmvk/Image.hpp"
#  include "../qmvk/Queue.hpp"
# endif

# ifdef Q_OS_ANDROID
#  include <QtAndroidExtras/QtAndroid>
# endif

# ifdef QMPLAY2_LIBASS
#  include <ass/ass.h>
# endif

# include <stdarg.h>

# ifndef Q_OS_WIN
#  include <sys/socket.h>
#  include <sys/ioctl.h>
#  include <sys/un.h>
# endif

# ifdef Q_OS_WIN
#  include <powrprof.h>
# endif

#endif

#ifdef QMPLAY2SHAREDLIB_LIBRARY
# define QMPLAY2SHAREDLIB_EXPORT Q_DECL_EXPORT
#else
# define QMPLAY2SHAREDLIB_EXPORT Q_DECL_IMPORT
#endif
