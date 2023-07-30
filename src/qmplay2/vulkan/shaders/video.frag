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
layout(constant_id = 13) const int trc = 0;

const int AVCOL_TRC_BT709 = 1;
const int AVCOL_TRC_SMPTE2084 = 16;

layout(location = 0) in vec2 inTextureCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform FragUniform
{
    mat3 yuvToRgbMatrix;
    mat3 colorPrimariesMatrix;

    vec2 levels;
    vec2 rangeMultiplier;
    float bitsMultiplier;

    float brightness;
    float contrast;
    float hue;
    float saturation;
    float sharpness;

    float maxLuminance;
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

// Bicubic filter is not written by me (https://www.shadertoy.com/view/MtVGWz)
vec3 getBicubic(in vec2 uv) // CatmullRom
{
    vec2 texSize = textureSize(inputImages[0], 0);

    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    vec2 samplePos = uv * texSize;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    vec2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / w12;

    // Compute the final UV coordinates we'll use for sampling the texture
    vec2 texPos0 = texPos1 - vec2(1.0);
    vec2 texPos3 = texPos1 + vec2(2.0);
    vec2 texPos12 = texPos1 + offset12;

    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

    return
        getBilinear(vec2(texPos0.x,  texPos0.y)) * w0.x  * w0.y +
        getBilinear(vec2(texPos12.x, texPos0.y)) * w12.x * w0.y +
        getBilinear(vec2(texPos3.x,  texPos0.y)) * w3.x  * w0.y +

        getBilinear(vec2(texPos0.x,  texPos12.y)) * w0.x  * w12.y +
        getBilinear(vec2(texPos12.x, texPos12.y)) * w12.x * w12.y +
        getBilinear(vec2(texPos3.x,  texPos12.y)) * w3.x  * w12.y +

        getBilinear(vec2(texPos0.x,  texPos3.y)) * w0.x  * w3.y +
        getBilinear(vec2(texPos12.x, texPos3.y)) * w12.x * w3.y +
        getBilinear(vec2(texPos3.x,  texPos3.y)) * w3.x  * w3.y
    ;
}

// HSL <-> RGB converter is not written by me
vec3 rgb2hsl(in vec3 c)
{
    const float eps = 1.19209e-07;

    float cmin = min(c.r, min(c.g, c.b));
    float cmax = max(c.r, max(c.g, c.b));
    float cd = cmax - cmin;

    vec3 a = vec3(1.0 - step(eps, abs(cmax - c)));
    a = mix(vec3(a.x, 0.0, a.z), a, step(0.5, 2.0 - a.x - a.y));
    a = mix(vec3(a.x, a.y, 0.0), a, step(0.5, 2.0 - a.x - a.z));
    a = mix(vec3(a.x, a.y, 0.0), a, step(0.5, 2.0 - a.y - a.z));

    vec3 hsl;
    hsl.z = (cmax + cmin) / 2.0;
    hsl.y = mix(cd / max(cmax + cmin, eps), cd / max(2.0 - (cmax + cmin), eps), step(0.5, hsl.z));
    hsl.x = dot(vec3(0.0, 2.0, 4.0) + ((c.gbr - c.brg) / max(cd, eps)), a);
    hsl.x = (hsl.x + (1.0 - step(0.0, hsl.x)) * 6.0) / 6.0;
    return hsl;
}
vec3 hsl2rgb(in vec3 c)
{
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

vec3 getPixel(in float ox, in float oy)
{
    return useBicubic
        ? getBicubic(inTextureCoord + vec2(ox, oy))
        : getBilinear(inTextureCoord + vec2(ox, oy))
    ;
}

// https://en.wikipedia.org/wiki/Perceptual_quantizer
vec3 smpte2084(in vec3 E)
{
    const float m1 = 1305.0 / 8192.0;
    const float m2 = 2523.0 / 32.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;
    const float c1 = c3 - c2 + 1.0;
    const vec3 E1m2 = pow(E, vec3(1.0 / m2));
    return pow(max(E1m2 - c1, 0.0) / (c2 - c3 * max(E1m2, 1e-6)), vec3(1.0 / m1));
}

// https://64.github.io/tonemapping/
float uncharted2_tonemap_partial(in float x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
float uncharted2_filmic(in float v)
{
    const float exposure_bias = 10.0;
    float curr = uncharted2_tonemap_partial(v * exposure_bias);
    float white_scale = 1.0 / uncharted2_tonemap_partial(7.5);
    return curr * white_scale;
}

// Tone curve according to Adobe's reference implementation (https://github.com/Beep6581/RawTherapee/blob/dev/rtengine/curves.h)
void rgbToneHelper(inout float maxVal, inout float medVal, inout float minVal)
{
    float minValOld = minVal;
    float medValOld = medVal;
    float maxValOld = maxVal;
    maxVal = uncharted2_filmic(maxValOld);
    minVal = uncharted2_filmic(minValOld);
    medVal = minVal + ((maxVal - minVal) * (medValOld - minValOld) / (maxValOld - minValOld));
}
void adobeFilmLikeUncharted2Filmic(inout vec3 rgb)
{
    if (rgb.r >= rgb.g)
    {
        if (rgb.g > rgb.b)
        {
            // Case 1: r >= g >  b
            rgbToneHelper(rgb.r, rgb.g, rgb.b);
        }
        else if (rgb.b > rgb.r)
        {
            // Case 2: b >  r >= g
            rgbToneHelper(rgb.b, rgb.r, rgb.g);
        }
        else if (rgb.b > rgb.g)
        {
            // Case 3: r >= b >  g
            rgbToneHelper(rgb.r, rgb.b, rgb.g);
        }
        else
        {
            // Case 4: r == g == b
            rgb.r = uncharted2_filmic(rgb.r);
            rgb.g = uncharted2_filmic(rgb.g);
            rgb.b = rgb.g;
        }
    }
    else
    {
        if (rgb.r >= rgb.b)
        {
            // Case 5: g >  r >= b
            rgbToneHelper(rgb.g, rgb.r, rgb.b);
        }
        else if (rgb.b >  rgb.g)
        {
            // Case 6: b >  g >  r
            rgbToneHelper(rgb.b, rgb.g, rgb.r);
        }
        else
        {
            // Case 7: g >= b >  r
            rgbToneHelper(rgb.g, rgb.b, rgb.r);
        }
    }
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

    if (!isGray && hasLuma)
    {
        value = clamp(yuvToRgbMatrix * value, 0.0, 1.0);
    }

    if (!isGray && !hasLuma && useHueSaturation)
    {
        value = rgb2hsl(value);
        value.x += hue;
        value.y *= saturation;
        value = clamp(hsl2rgb(value), 0.0, 1.0);
    }

    if (trc == AVCOL_TRC_BT709)
    {
        value = pow(value, vec3(2.4));
        value = clamp(colorPrimariesMatrix * value, 0.0, 1.0);
        value = pow(value, vec3(1.0 / 2.4));
    }
    else if (trc == AVCOL_TRC_SMPTE2084)
    {
        value = smpte2084(value);
        value *= 10000.0 / maxLuminance;
        value = clamp(value, 0.0, 1.0);
        adobeFilmLikeUncharted2Filmic(value);
        value = colorPrimariesMatrix * value;
        value = clamp(value, 0.0, 1.0);
        value = pow(value, vec3(1.0 / 2.4));
    }

    if (useBrightnessContrast)
    {
        value = (value - 0.5) * contrast + 0.5 + brightness;
    }

    outColor = vec4(value, 1.0);
}
