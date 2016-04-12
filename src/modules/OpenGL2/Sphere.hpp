#ifndef SPHERE_HPP
#define SPHERE_HPP

#include <QtGlobal>

namespace Sphere
{
	quint32 getSizes(quint32 slices, quint32 stacks, quint32 &nVertices, quint32 &nTexcoords, quint32 &nIndices);
	void generate(float radius, quint32 slices, quint32 stacks, float *vertices, float *texcoords, quint16 *indices);
}

#endif // SPHERE_HPP
