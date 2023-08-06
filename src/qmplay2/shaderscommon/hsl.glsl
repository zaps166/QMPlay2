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
