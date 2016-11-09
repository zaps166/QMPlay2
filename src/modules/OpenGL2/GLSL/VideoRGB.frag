varying vec2 vTexCoord;
uniform sampler2D uRGB;

void main()
{
	gl_FragColor = vec4(texture2D(uRGB, vTexCoord).rgb, 1.0);
}
