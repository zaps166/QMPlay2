layout(constant_id =  0) const int numPlanes = 3;
layout(constant_id =  1) const int idxPlane0 = 0;
layout(constant_id =  2) const int idxPlane1 = 0;
layout(constant_id =  3) const int idxPlane2 = 0;
layout(constant_id =  4) const int idxComponent0 = 0;
layout(constant_id =  5) const int idxComponent1 = 0;
layout(constant_id =  6) const int idxComponent2 = 0;
layout(constant_id =  7) const bool hasLuma = false;
layout(constant_id =  8) const bool isGray = false;
layout(constant_id =  9) const bool useBicubic = false;
layout(constant_id = 10) const bool useBrightnessContrast = false;
layout(constant_id = 11) const bool useHueSaturation = false;
layout(constant_id = 12) const bool useSharpness = false;

layout(location = 0) in vec2 inTextureCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform FragUniform
{
    mat3 conversionMatrix;
    vec2 levels;
    vec2 rangeMultiplier;
    float bitsMultiplier;

    float brightness;
    float contrast;
    float hue;
    float saturation;
    float sharpness;
};
layout(binding = 1) uniform sampler2D inputImages[numPlanes];

vec3 getBilinear(in vec2 pos)
{
    if (numPlanes == 1)
    {
        vec4 value = texture(inputImages[0], pos);
        return vec3(
            value[idxComponent0],
            value[idxComponent1],
            value[idxComponent2]
        );
    }
    else if (numPlanes == 2)
    {
        vec4 value = texture(inputImages[idxPlane1], pos);
        return vec3(
            texture(inputImages[idxPlane0], pos)[0],
            value[idxComponent1],
            value[idxComponent2]
        );
    }
    else if (numPlanes == 3)
    {
        return vec3(
            texture(inputImages[idxPlane0], pos)[0],
            texture(inputImages[idxPlane1], pos)[0],
            texture(inputImages[idxPlane2], pos)[0]
        );
    }
    return vec3(0.0, 0.0, 0.0);
}

// Bicubic filter is not written by me
vec4 cubic(in float v)
{
    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return vec4(x, y, z, w) * (1.0 / 6.0);
}
vec3 getBicubic(in vec2 inPos)
{
    vec2 size = textureSize(inputImages[0], 0);
    vec2 invSize = 1.0 / size;

    vec2 pos = (inPos * size) - 0.5;

    vec2 fxy = fract(pos);
    pos -= fxy;

    vec4 xcubic = cubic(fxy.x);
    vec4 ycubic = cubic(fxy.y);

    vec4 c = pos.xxyy + vec2(-0.5, +1.5).xyxy;

    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 o = (c + vec4 (xcubic.yw, ycubic.yw) / s) * invSize.xxyy;

    vec3 sample0 = getBilinear(o.xz);
    vec3 sample1 = getBilinear(o.yz);
    vec3 sample2 = getBilinear(o.xw);
    vec3 sample3 = getBilinear(o.yw);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

// HSV <-> RGB converter is not written by me
vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 getPixel(in float ox, in float oy)
{
    return useBicubic
        ? getBicubic(inTextureCoord + vec2(ox, oy))
        : getBilinear(inTextureCoord + vec2(ox, oy))
    ;
}

void main()
{
    vec3 value = getPixel(0.0, 0.0);

    if (useSharpness)
    {
        vec2 single = 1.0 / textureSize(inputImages[0], 0);
        if (hasLuma && !isGray)
        {
            float blur = (
                getPixel(-single.x, -single.y)[0] / 16.0 + getPixel(0.0, -single.y)[0] / 8.0 + getPixel(single.x, -single.y)[0] / 16.0 +
                getPixel(-single.x,       0.0)[0] /  8.0 + value[0]                    / 4.0 + getPixel(single.x,       0.0)[0] /  8.0 +
                getPixel(-single.x,  single.y)[0] / 16.0 + getPixel(0.0,  single.y)[0] / 8.0 + getPixel(single.x,  single.y)[0] / 16.0
            );
            value[0] = clamp(value[0] + (value[0] - blur) * sharpness, 0.0, 1.0);
        }
        else
        {
            vec3 blur = (
                getPixel(-single.x, -single.y) / 16.0 + getPixel(0.0, -single.y) / 8.0 + getPixel(single.x, -single.y) / 16.0 +
                getPixel(-single.x,  0.0     ) /  8.0 + value                    / 4.0 + getPixel(single.x,  0.0     ) /  8.0 +
                getPixel(-single.x,  single.y) / 16.0 + getPixel(0.0,  single.y) / 8.0 + getPixel(single.x,  single.y) / 16.0
            );
            value = clamp(value + (value - blur) * sharpness, 0.0, 1.0);
        }
    }

    value = ((value * bitsMultiplier) - levels.xyy) * rangeMultiplier.xyy;

    if (!isGray && hasLuma && useHueSaturation)
    {
        value.yz *= saturation;
        if (hue != 0.0)
        {
            float hue = atan(value[2], value[1]) - (hue * 2.0 * 3.141592);
            float chroma = sqrt(value[1] * value[1] + value[2] * value[2]);
            value[1] = chroma * cos(hue);
            value[2] = chroma * sin(hue);
        }
    }

    value = conversionMatrix * value;

    if (!isGray && !hasLuma && useHueSaturation)
    {
        value = rgb2hsv(value);
        value.x += hue;
        value.y *= saturation;
        value = hsv2rgb(value);
    }

    if (useBrightnessContrast)
    {
        value = (value - 0.5) * contrast + 0.5 + brightness;
    }

    outColor = vec4(value, 1.0);
}
