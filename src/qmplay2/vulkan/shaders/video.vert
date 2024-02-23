layout(push_constant) uniform PushConstants
{
    mat4 matrix;
    vec2 imgSizeMultiplier;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoord;

layout(location = 0) out vec2 outTextureCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = matrix * vec4(inPosition, 1.0);
    outTextureCoord = inTextureCoord * imgSizeMultiplier;
}
