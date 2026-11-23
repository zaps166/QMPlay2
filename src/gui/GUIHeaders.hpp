// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef Q_MOC_RUN

# include <QAbstractNativeEventFilter>
# include <QOperatingSystemVersion>
# include <QAbstractTableModel>
# include <QCommandLineParser>
# include <QCryptographicHash>
# include <QFileSystemWatcher>
# include <QRandomGenerator>
# include <QVarLengthArray>
# include <QStandardPaths>
# include <QVersionNumber>
# include <QWaitCondition>
# include <QStringList>
# include <QAtomicInt>
# include <QDate>
# ifdef Q_OS_ANDROID
#  include <QJniObject>
# endif

# include <QTextCodec>
# include <QMetaEnum>
# include <QMimeData>
# include <QLibrary>
# include <QProcess>
# include <QSysInfo>
# include <QBuffer>
# include <QThread>
# include <QDebug>
# include <QEvent>
# include <QTimer>
# include <QSize>
# include <QUrl>

# include <QDesktopServices>
# include <QGuiApplication>
# include <QSurfaceFormat>
# include <QActionGroup>
# include <QImageReader>
# include <QImageWriter>
# include <QResizeEvent>
# include <QMouseEvent>
# include <QClipboard>
# include <QShortcut>
# include <QPainter>
# include <QRawFont>
# include <QAction>
# include <QScreen>
# include <QWindow>
# include <QImage>
# include <QDrag>

# include <QAbstractItemView>
# include <QDialogButtonBox>
# include <QDoubleSpinBox>
# include <QPlainTextEdit>
# include <QStackedWidget>
# include <QFontComboBox>
# include <QStyleFactory>
# include <QWidgetAction>
# include <QApplication>
# include <QCommonStyle>
# include <QInputDialog>
# include <QProgressBar>
# include <QRadioButton>
# include <QStyleOption>
# include <QDockWidget>
# include <QFileDialog>
# include <QFormLayout>
# include <QGridLayout>
# include <QHBoxLayout>
# include <QHeaderView>
# include <QListWidget>
# include <QMainWindow>
# include <QMessageBox>
# include <QProxyStyle>
# include <QPushButton>
# include <QScrollArea>
# include <QSpacerItem>
# include <QToolButton>
# include <QTreeWidget>
# include <QVBoxLayout>
# include <QBoxLayout>
# include <QScrollBar>
# include <QStatusBar>
# include <QTabWidget>
# include <QTableView>
# include <QCheckBox>
# include <QComboBox>
# include <QGroupBox>
# include <QLineEdit>
# include <QScroller>
# include <QTextEdit>
# include <QGesture>
# include <QMenuBar>
# include <QSpinBox>
# include <QToolBar>
# include <QDialog>
# include <QSlider>
# include <QWidget>
# include <QFrame>
# include <QLabel>
# include <QStyle>
# include <QMenu>

# ifdef DBUS_PM
#  include <QDBusConnection>
#  include <QDBusInterface>
# endif

# include <map>

# ifdef USE_VULKAN
#  include "../qmvk/AbstractInstance.hpp"
#  include "../qmvk/ComputePipeline.hpp"
#  include "../qmvk/PhysicalDevice.hpp"
#  include "../qmvk/Sampler.hpp"
# endif

# ifdef QMPlay2_TagEditor
#  ifdef TAGLIB_FULL_INCLUDE_PATH
#   include <taglib/taglib.h>
#  else
#   include <taglib.h>
#  endif
#  ifdef TAGLIB_FULL_INCLUDE_PATH
#   include <taglib/trueaudiofile.h>
#   include <taglib/oggflacfile.h>
#   include <taglib/wavpackfile.h>
#   include <taglib/vorbisfile.h>
#   include <taglib/speexfile.h>
#   include <taglib/aifffile.h>
#   include <taglib/mpegfile.h>
#   include <taglib/flacfile.h>
#   include <taglib/asffile.h>
#   include <taglib/mpcfile.h>
#   include <taglib/mp4file.h>
#   include <taglib/wavfile.h>
#   include <taglib/apefile.h>
#   include <taglib/fileref.h>
#   include <taglib/modfile.h>
#   include <taglib/s3mfile.h>
#   include <taglib/itfile.h>
#   include <taglib/xmfile.h>
#   include <taglib/opusfile.h>
#   include <taglib/attachedpictureframe.h>
#  else
#   include <trueaudiofile.h>
#   include <oggflacfile.h>
#   include <wavpackfile.h>
#   include <vorbisfile.h>
#   include <speexfile.h>
#   include <aifffile.h>
#   include <mpegfile.h>
#   include <flacfile.h>
#   include <asffile.h>
#   include <mpcfile.h>
#   include <mp4file.h>
#   include <wavfile.h>
#   include <apefile.h>
#   include <fileref.h>
#   include <modfile.h>
#   include <s3mfile.h>
#   include <itfile.h>
#   include <xmfile.h>
#   include <opusfile.h>
#   include <attachedpictureframe.h>
#  endif
# endif

# include <signal.h>
# include <clocale>

# ifdef Q_OS_WIN
#  include <lmcons.h>
#  include <QtWinExtras/qwinthumbnailtoolbutton.h>
#  include <QtWinExtras/qwinthumbnailtoolbar.h>
#  include <QtWinExtras/qwintaskbarprogress.h>
#  include <QtWinExtras/qwintaskbarbutton.h>
#  if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#   include <dwmapi.h>
#  endif
# endif

# define MUTEXWAIT_TIMEOUT 1250
# define TERMINATE_TIMEOUT (MUTEXWAIT_TIMEOUT*3)

#endif
