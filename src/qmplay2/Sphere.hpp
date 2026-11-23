// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

namespace Sphere
{
    QMPLAY2SHAREDLIB_EXPORT quint32 getSizes(quint32 slices, quint32 stacks, quint32 &verticesSize, quint32 &texcoordsSize, quint32 &indicesSize);
    QMPLAY2SHAREDLIB_EXPORT void generate(float radius, quint32 slices, quint32 stacks, float *vertices, float *texcoords, quint16 *indices);
}
