#include "Sphere.hpp"

#include <math.h>

quint32 Sphere::getSizes(quint32 slices, quint32 stacks, quint32 &nVertices, quint32 &nTexcoords, quint32 &nIndices)
{
	nVertices  = slices * stacks * 3 * sizeof(float);
	nTexcoords = slices * stacks * 2 * sizeof(float);
	nIndices   = slices * stacks * 2 * sizeof(quint16);
	return nIndices / sizeof(quint16);
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

			*(indices++) = (stack + 0) * slices + slice;
			*(indices++) = (stack + 1) * slices + slice;
		}
	}
}
