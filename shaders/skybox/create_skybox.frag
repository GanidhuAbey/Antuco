#version 450
#extension GL_KHR_vulkan_glsl: enable

//layout(location = 1) in vec3 localPos;

layout(location=0) out vec4 out_color;

//layout(set=0, binding=0) uniform sampler2D equirectangularMap;

// Credit: LearnOpenGl tutorial
const vec2 invAtan =  vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;

    return uv;
}

void main() {
    //vec2 uv = SampleSphericalMap(normalize(localPos));
    out_color = vec4(0.0, 1.0, 0.0, 0.0); //vec4(texture(equirectangularMap, uv).rgb, 1.0);
}
