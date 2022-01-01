/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Sphere.hpp>

#include <cmath>

using namespace std;

quint32 Sphere::getSizes(quint32 slices, quint32 stacks, quint32 &verticesSize, quint32 &texcoordsSize, quint32 &indicesSize)
{
    verticesSize  = slices * stacks * 3 * sizeof(float);
    texcoordsSize = slices * stacks * 2 * sizeof(float);
    indicesSize   = slices * (stacks - 1) * 2 * sizeof(quint16);
    return indicesSize / sizeof(quint16);
}
void Sphere::generate(float radius, quint32 slices, quint32 stacks, float *vertices, float *texcoords, quint16 *indices)
{
    const double iStacks = 1.0 / (stacks - 1.0);
    const double iSlices = 1.0 / (slices - 1.0);
    for (quint32 stack = 0; stack < stacks; ++stack)
    {
        for (quint32 slice = 0; slice < slices; ++slice)
        {
            const double theta = stack * M_PI * iStacks;
            const double phi = slice * 2.0 * M_PI * iSlices;
            const double sinTheta = sin(theta);
            const double cosTheta = cos(theta);
            const double sinPhi = sin(phi);
            const double cosPhi = cos(phi);

            *(vertices++) = radius * cosPhi * sinTheta;
            *(vertices++) = radius * sinPhi * sinTheta;
            *(vertices++) = radius * cosTheta;

            *(texcoords++) = slice * iSlices;
            *(texcoords++) = (stacks - stack - 1) * iStacks;

            if (stack < stacks - 1)
            {
                *(indices++) = (stack + 0) * slices + slice;
                *(indices++) = (stack + 1) * slices + slice;
            }
        }
    }
}
