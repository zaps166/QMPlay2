#ifndef SPHERE_HPP
#define SPHERE_HPP

#include <QtGlobal>

namespace Sphere
{
	quint32 getSizes(quint32 slices, quint32 stacks, quint32 &verticesSize, quint32 &texcoordsSize, quint32 &indicesSize);
	void generate(float radius, quint32 slices, quint32 stacks, float *vertices, float *texcoords, quint16 *indices);
}

#endif // SPHERE_HPP
