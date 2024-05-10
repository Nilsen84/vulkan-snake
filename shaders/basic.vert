#version 450

const vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

layout (push_constant) uniform PushConstantsVertex {
    mat4 mvp;
} pc;

void main() {
    gl_Position = pc.mvp * vec4(positions[gl_VertexIndex], 0.0, 1.0);
}