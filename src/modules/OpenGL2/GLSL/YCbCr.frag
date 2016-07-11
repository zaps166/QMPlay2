varying vec2 vTexCoord;
uniform vec4 uVideoEq;
uniform sampler2D uY, uCb, uCr;

void main()
{
	float brightness = uVideoEq[0];
	float contrast = uVideoEq[1];
	float saturation = uVideoEq[2];
	vec3 YCbCr = vec3(
		texture2D(uY , vTexCoord)[0] - 0.0625,
		texture2D(uCb, vTexCoord)[0] - 0.5,
		texture2D(uCr, vTexCoord)[0] - 0.5
	);
	/* Hue
	float hueAdj = uVideoEq[3];
	if (hueAdj != 0.0)
	{
		float hue = atan(YCbCr[2], YCbCr[1]) + hueAdj;
		float chroma = sqrt(YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2]);
		YCbCr[1] = chroma * cos(hue);
		YCbCr[2] = chroma * sin(hue);
	}
	Hue */
	YCbCr.yz *= saturation;
	vec3 rgb = mat3(
		1.16430,  1.16430, 1.16430,
		0.00000, -0.39173, 2.01700,
		1.59580, -0.81290, 0.00000
	) * YCbCr * contrast + brightness;
	gl_FragColor = vec4(rgb, 1.0);
}
