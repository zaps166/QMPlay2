#ifndef sampler
    #define sampler sampler2D
#endif

uniform int uBicubic;

// w0, w1, w2, and w3 are the four cubic B-spline basis functions
float w0(float a)
{
    return (1.0 / 6.0) * (a * (a * (-a + 3.0) - 3.0) + 1.0);
}
float w1(float a)
{
    return (1.0 / 6.0) * (a * a * (3.0 * a - 6.0) + 4.0);
}
float w2(float a)
{
    return (1.0 / 6.0) * (a * (a * (-3.0 * a + 3.0) + 3.0) + 1.0);
}
float w3(float a)
{
    return (1.0 / 6.0) * (a * a * a);
}

// g0 and g1 are the two amplitude functions
float g0(float a)
{
    return w0(a) + w1(a);
}
float g1(float a)
{
    return w2(a) + w3(a);
}

// h0 and h1 are the two offset functions
float h0(float a)
{
    return -1.0 + w1(a) / (w0(a) + w1(a));
}
float h1(float a)
{
    return 1.0 + w3(a) / (w2(a) + w3(a));
}

vec4 getTexel(sampler tex, vec2 st)
{
    if (uBicubic == 0)
        return texture(tex, st);

    st = st * uTextureSize + 0.5;

    vec2 iSt = floor(st);
    vec2 fSt = fract(st);

    float g0x = g0(fSt.x);
    float g1x = g1(fSt.x);
    float g0y = g0(fSt.y);
    float g1y = g1(fSt.y);
    float h0x = h0(fSt.x);
    float h1x = h1(fSt.x);
    float h0y = h0(fSt.y);
    float h1y = h1(fSt.y);

    vec2 p0 = (vec2(iSt.x + h0x, iSt.y + h0y) - 0.5) / uTextureSize;
    vec2 p1 = (vec2(iSt.x + h1x, iSt.y + h0y) - 0.5) / uTextureSize;
    vec2 p2 = (vec2(iSt.x + h0x, iSt.y + h1y) - 0.5) / uTextureSize;
    vec2 p3 = (vec2(iSt.x + h1x, iSt.y + h1y) - 0.5) / uTextureSize;

    return g0y * (g0x * texture(tex, p0) + g1x * texture(tex, p1)) +
           g1y * (g0x * texture(tex, p2) + g1x * texture(tex, p3));
}
