varying vec2 vTexCoord;
uniform float uSharpness;
uniform vec2 uStep;
uniform sampler2D uRGB;

#ifdef Sharpness
vec4 getRGBAtOffset(float x, float y)
{
	return texture2D(uRGB, vTexCoord + vec2(x, y));
}
#endif

void main()
{
	vec4 RGB = texture2D(uRGB, vTexCoord);
#ifdef Sharpness
	if (uSharpness != 0.0)
	{
		// Kernel 3x3
		// 1 2 1
		// 2 4 2
		// 1 2 1
		vec4 blur = (
			getRGBAtOffset(-uStep.x, -uStep.y) / 16.0 + getRGBAtOffset(0.0, -uStep.y) / 8.0 + getRGBAtOffset(uStep.x, -uStep.y) / 16.0 +
			getRGBAtOffset(-uStep.x,  0.0    ) /  8.0 + getRGBAtOffset(0.0,  0.0    ) / 4.0 + getRGBAtOffset(uStep.x,  0.0    ) /  8.0 +
			getRGBAtOffset(-uStep.x,  uStep.y) / 16.0 + getRGBAtOffset(0.0,  uStep.y) / 8.0 + getRGBAtOffset(uStep.x,  uStep.y) / 16.0
		);
		// Subtract blur from original image, multiply and then add it to the original image
		RGB = clamp(RGB + (RGB - blur) * uSharpness, 0.0, 1.0);
	}
#endif
	gl_FragColor = RGB;
}
