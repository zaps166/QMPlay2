attribute vec4 aPosition;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = aPosition;
}
