varying vec2 vTexCoord;
uniform sampler2D uTex;

void main()
{
    gl_FragColor = texture2D(uTex, vTexCoord);
}
