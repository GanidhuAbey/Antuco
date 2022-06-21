#version 450

layout(set=0, binding=0) uniform sampler2D output_texture;

layout(location=1) in vec2 uv;

layout(location=0) out vec4 out_color;

void main() {
    vec4 lin_col = texture(output_texture, uv);
    out_color = lin_col;
}
