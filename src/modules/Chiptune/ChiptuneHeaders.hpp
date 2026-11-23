// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef Q_MOC_RUN

# include <QGridLayout>
# include <QCheckBox>
# include <QSpinBox>

# ifdef USE_SIDPLAY
#  include <sidplayfp/sidplayfp.h>
#  if LIBSIDPLAYFP_VERSION_MAJ >= 3
#   include <sidplayfp/builders/sidlite.h>
#  else
#   include <sidplayfp/builders/residfp.h>
#  endif
#  include <sidplayfp/SidConfig.h>
#  include <sidplayfp/SidInfo.h>
#  include <sidplayfp/SidTune.h>
#  include <sidplayfp/SidTuneInfo.h>
# endif

# ifdef USE_GME
#  include <gme/gme.h>
# endif

#endif
