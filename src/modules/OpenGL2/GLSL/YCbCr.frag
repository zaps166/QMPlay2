varying vec2 vTexCoord;
uniform vec4 uVideoEq;
uniform float uSharpness;
uniform vec2 uStep;
uniform sampler2D uY;
#ifdef NV12
	uniform sampler2D uCbCr;
#else
	uniform sampler2D uCb, uCr;
#endif

const mat3 YUVtoRGB = mat3(
	1.16430,  1.16430, 1.16430,
	0.00000, -0.39173, 2.01700,
	1.59580, -0.81290, 0.00000
);

#ifdef HueAndSharpness
float getLumaAtOffset(float x, float y)
{
	return texture2D(uY, vTexCoord + vec2(x, y))[0] - 0.0625;
}
#endif

void main()
{
	float brightness = uVideoEq[0];
	vec3 contrastSaturation = vec3(
		uVideoEq[1],
		uVideoEq[1] * uVideoEq[2],
		uVideoEq[1] * uVideoEq[2]
	);

#ifdef NV12
	vec3 YCbCr = vec3(
		texture2D(uY   , vTexCoord)[0] - 0.0625,
		texture2D(uCbCr, vTexCoord)[0] - 0.5,
		texture2D(uCbCr, vTexCoord)[1] - 0.5
	);
#else
	vec3 YCbCr = vec3(
		texture2D(uY , vTexCoord)[0] - 0.0625,
		texture2D(uCb, vTexCoord)[0] - 0.5,
		texture2D(uCr, vTexCoord)[0] - 0.5
	);
#endif

#ifdef HueAndSharpness
	if (uSharpness != 0.0)
	{
		// Kernel 3x3
		// 1 2 1
		// 2 4 2
		// 1 2 1
		float lumaBlur = (
			getLumaAtOffset(-uStep.x, -uStep.y) / 16.0 + getLumaAtOffset(0.0, -uStep.y) / 8.0 + getLumaAtOffset(uStep.x, -uStep.y) / 16.0 +
			getLumaAtOffset(-uStep.x,  0.0    ) /  8.0 + getLumaAtOffset(0.0,  0.0    ) / 4.0 + getLumaAtOffset(uStep.x,  0.0    ) /  8.0 +
			getLumaAtOffset(-uStep.x,  uStep.y) / 16.0 + getLumaAtOffset(0.0,  uStep.y) / 8.0 + getLumaAtOffset(uStep.x,  uStep.y) / 16.0
		);
		// Subtract blur from original image, multiply and then add it to the original image
		YCbCr[0] = clamp(YCbCr[0] + (YCbCr[0] - lumaBlur) * uSharpness, 0.0, 1.0);
	}

	float hueAdj = uVideoEq[3];
	if (hueAdj != 0.0)
	{
		float hue = atan(YCbCr[2], YCbCr[1]) + hueAdj;
		float chroma = sqrt(YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2]);
		YCbCr[1] = chroma * cos(hue);
		YCbCr[2] = chroma * sin(hue);
	}
#endif

	gl_FragColor = vec4(clamp(YUVtoRGB * (YCbCr * contrastSaturation), 0.0, 1.0) + brightness, 1.0);
}
