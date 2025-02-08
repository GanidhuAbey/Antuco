#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(set=0, binding=1) uniform samplerCube cubemap;

layout(location=1) in vec3 localPos;

layout(location=0) out vec4 out_color;

void main() {
    vec3 viewDirection = localPos;
    out_color = texture(cubemap, viewDirection);
}
