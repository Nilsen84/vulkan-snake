#version 450

layout (location = 0) out vec4 o_FragColor;

layout (push_constant) uniform PushConstantsFragment {
    layout (offset = 64) vec3 color;
} pc;

void main()
{
    o_FragColor = vec4(pc.color, 1.0f);
}