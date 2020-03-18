layout(push_constant) uniform PushConstants
{
    mat4 matrix;
    ivec2 size;
    int linesize;
    vec4 color;
};

layout(location = 0) in vec2 inDataCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform textureBuffer osdData;

void main()
{
    ivec2 pos = ivec2(ceil(inDataCoord * size));
    float value = texelFetch(osdData, linesize * pos.y + pos.x)[0];
    outColor = vec4(color.rgb, color.a * value);
}
