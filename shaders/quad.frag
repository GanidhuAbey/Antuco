#version 450

layout(set=0, binding=0) uniform sampler2D output_texture;

layout(location=1) in vec2 uv;

layout(location=0) out vec4 out_color;

void main() {
    out_color = texture(output_texture, uv);
}
