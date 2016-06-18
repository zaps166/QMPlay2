/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

static const char vShaderYCbCrSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"attribute vec4 aPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"uniform mat4 matrix;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = matrix * aPosition;"
	"}";
static const char fShaderYCbCrSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"varying vec2 vTexCoord;"
	"uniform vec4 videoEq;"
	"uniform sampler2D Ytex, Utex, Vtex;"
	"void main() {"
		"float brightness = videoEq[0];"
		"float contrast = videoEq[1];"
		"float saturation = videoEq[2];"
		"vec3 YCbCr = vec3("
			"texture2D(Ytex, vTexCoord)[0] - 0.0625,"
			"texture2D(Utex, vTexCoord)[0] - 0.5,"
			"texture2D(Vtex, vTexCoord)[0] - 0.5"
		");"
		"%1"
		"YCbCr.yz *= saturation;"
		"vec3 rgb = mat3"
		"("
			"1.16430,  1.16430, 1.16430,"
			"0.00000, -0.39173, 2.01700,"
			"1.59580, -0.81290, 0.00000"
		") * YCbCr * contrast + brightness;"
		"gl_FragColor = vec4(rgb, 1.0);"
	"}";
static const char fShaderYCbCrHueSrc[] =
	"float hueAdj = videoEq[3];"
	"if (hueAdj != 0.0) {"
		"float hue = atan(YCbCr[2], YCbCr[1]) + hueAdj;"
		"float chroma = sqrt(YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2]);"
		"YCbCr[1] = chroma * cos(hue);"
		"YCbCr[2] = chroma * sin(hue);"
	"}";

static const char vShaderOSDSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"attribute vec4 aPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = aPosition;"
	"}";
static const char fShaderOSDSrc[] =
#ifdef OPENGL_ES2
	"precision lowp float;"
#endif
	"varying vec2 vTexCoord;"
	"uniform sampler2D tex;"
	"void main() {"
		"gl_FragColor = texture2D(tex, vTexCoord);"
	"}";
