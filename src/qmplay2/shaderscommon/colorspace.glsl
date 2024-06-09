const int AVCOL_TRC_BT709 = 1;
const int AVCOL_TRC_SMPTE2084 = 16;
const int AVCOL_TRC_ARIB_STD_B67 = 18;

// https://en.wikipedia.org/wiki/Perceptual_quantizer
vec3 smpte2084(in vec3 E)
{
    const float m1 = 1305.0 / 8192.0;
    const float m2 = 2523.0 / 32.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;
    const float c1 = c3 - c2 + 1.0;
    vec3 E1m2 = pow(E, vec3(1.0 / m2));
    return pow(max(E1m2 - c1, 0.0) / (c2 - c3 * max(E1m2, 1e-6)), vec3(1.0 / m1));
}

const float ARIB_B67_A = 0.17883277;
const float ARIB_B67_B = 0.28466892;
const float ARIB_B67_C = 0.55991073;
vec3 arib_b67(in vec3 val)
{
    for (int i = 0; i < 3; ++i)
    {
        if (val[i] <= 0.5)
            val[i] = (val[i] * val[i]) * (1.0 / 3.0);
        else
            val[i] = (exp((val[i] - ARIB_B67_C) / ARIB_B67_A) + ARIB_B67_B) / 12.0;
    }
    return val;
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

void colorspace_trc_bt709(inout vec3 value, in mat3 colorPrimariesMatrix)
{
    value = pow(value, vec3(2.4));
    value = clamp(colorPrimariesMatrix * value, 0.0, 1.0);
    value = pow(value, vec3(1.0 / 2.4));
}
void colorspace_trc_smpte2084(inout vec3 value, in mat3 colorPrimariesMatrix, in float maxLuminance)
{
    value = smpte2084(value);
    value *= 10000.0 / maxLuminance;
    value = clamp(value, 0.0, 1.0);
    adobeFilmLikeUncharted2Filmic(value);
    value = colorPrimariesMatrix * value;
    value = clamp(value, 0.0, 1.0);
    value = pow(value, vec3(1.0 / 2.4));
}

void colorspace_trc_hlg(inout vec3 value, in mat3 colorPrimariesMatrix, in float maxLuminance)
{
    // TODO: maxLuminance
    value = arib_b67(value);
    value = clamp(value, 0.0, 1.0);
    adobeFilmLikeUncharted2Filmic(value);
    value = colorPrimariesMatrix * value;
    value = clamp(value, 0.0, 1.0);
    value = pow(value, vec3(1.0 / 1.8));
}
