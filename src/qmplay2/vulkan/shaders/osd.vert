layout(push_constant) uniform PushConstants
{
    mat4 matrix;
};

const vec2 g_positions[4] = vec2[](
    vec2(0.0, 1.0), // bottom left
    vec2(0.0, 0.0), // top left
    vec2(1.0, 1.0), // bottom right
    vec2(1.0, 0.0)  // top right
);

layout(location = 0) out vec2 outDataCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = matrix * vec4(g_positions[gl_VertexIndex], 0.0, 1.0);
    outDataCoord = g_positions[gl_VertexIndex];
}
