#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(set=0, binding=0) uniform sampler2D skyboxTexture;

layout(location = 2) in vec3 surfaceNormal;
layout(location = 1) in vec2 texCoord;
layout(location = 3) in vec4 vPos;
layout(location = 4) in vec3 camera_pos;

layout(location=0) out vec4 out_color;

void main() {
    //vec3 viewDirection = normalize(camera_pos - vec3(vPos));
    out_color = texture(skyboxTexture, texCoord);
}
