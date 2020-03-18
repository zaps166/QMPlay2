layout(push_constant) uniform PushConstants
{
    mat4 matrix;
    ivec2 size;
    int linesize;
};

layout(location = 0) in vec2 inDataCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform textureBuffer palette;
layout(binding = 1) uniform utextureBuffer data;

vec4 getColorAt(in ivec2 pos)
{
    return texelFetch(
        palette,
        int(texelFetch(data, linesize * pos.y + pos.x))
    );
}

void main()
{
    vec2 pos = inDataCoord * size;
    ivec2 iPos = ivec2(pos);
    vec2 posFract = fract(pos);

    ivec2 next = ivec2(
        (size.x - iPos.x) != 1,
        (size.y - iPos.y) != 1
    );

    vec4 c00 = getColorAt(iPos + ivec2(0,      0));
    vec4 c10 = getColorAt(iPos + ivec2(next.x, 0));
    vec4 c01 = getColorAt(iPos + ivec2(0,      next.y));
    vec4 c11 = getColorAt(iPos + ivec2(next.x, next.y));

    vec4 color = mix(
        mix(c00 * vec4(c00.aaa, 1.0), c10 * vec4(c10.aaa, 1.0), posFract.x),
        mix(c01 * vec4(c01.aaa, 1.0), c11 * vec4(c11.aaa, 1.0), posFract.x),
        posFract.y
    );
    outColor = vec4(color.rgb / color.a, color.a);
}
