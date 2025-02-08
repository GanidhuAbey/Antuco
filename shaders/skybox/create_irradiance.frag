#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(set=0, binding=1) uniform samplerCube cubemap;

layout(location=1) in vec3 localPos;

layout(location=0) out vec4 out_color;

const float PI = 3.14159265359;

void main() {
    vec3 out_direction = normalize(localPos);
    vec3 irradiance = vec3(0.0);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, out_direction));
    up = normalize(cross(out_direction, right));

    float delta = 0.025;
    float samples = 0.0;
    for (float theta = 0.0; theta < PI / 2; theta += delta) {
        for (float phi = 0.0; phi < 2 * PI; phi += delta) {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * out_direction;

            irradiance += texture(cubemap, sampleVec).rgb * cos(theta) * sin(theta);
            samples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / samples);

    out_color = vec4(irradiance, 1.0);
}
