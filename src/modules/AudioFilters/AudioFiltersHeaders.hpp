// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef Q_MOC_RUN

# include <QVarLengthArray>
# include <QDebug>

# include <QPainterPath>
# include <QPainter>

# include <QDoubleSpinBox>
# include <QInputDialog>
# include <QTextBrowser>
# include <QDockWidget>
# include <QFormLayout>
# include <QGridLayout>
# include <QMessageBox>
# include <QPushButton>
# include <QScrollArea>
# include <QToolButton>
# include <QBoxLayout>
# include <QCheckBox>
# include <QComboBox>
# include <QGroupBox>
# include <QLineEdit>
# include <QSpinBox>
# include <QSlider>
# include <QLabel>
# include <QMenu>

#  ifdef USE_AVAUDIOFILTER
extern "C" {
#   include <libavfilter/buffersink.h>
#   include <libavfilter/buffersrc.h>
}
#  endif

# include <bs2b/bs2b.hpp>

#endif
