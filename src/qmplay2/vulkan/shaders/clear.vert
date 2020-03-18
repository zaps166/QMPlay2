const vec2 g_positions[4] = vec2[](
    vec2(-1.0,  1.0), // bottom left
    vec2(-1.0, -1.0), // top left
    vec2( 1.0,  1.0), // bottom right
    vec2( 1.0, -1.0)  // top right
);

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = vec4(g_positions[gl_VertexIndex], 0.0, 1.0);
}
