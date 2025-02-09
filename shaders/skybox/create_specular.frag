#version 450
#extension GL_KHR_vulkan_glsl: enable

layout(set=0, binding=1) uniform samplerCube cubemap;

layout(location=1) in vec3 localPos;

layout(location=0) out vec4 out_color;

const float PI = 3.14159265359;

// TODO: need to pass this in via descriptor
float roughness = 0.1;


// To ensure that our importance sampling generates samples that are more evenly distributed, we use a low-discrepancy sequence.

// This generates a Van Der Corput sequence (https://en.wikipedia.org/wiki/Van_der_Corput_sequence)
float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// Using the Van Der Corput sequence, we can generate a Hammersley Sequence which we use as our low-discrepancy sequence
// (good reference: https://www.shadertoy.com/view/4lscWj)
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampling_GGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2 * PI * Xi.x;
    float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    // spherical -> cartesion coordinates (TODO : need refresher on this...)
    vec3 H;
    H.x = cos(phi) * sin_theta;
    H.y = sin(phi) * sin_theta;
    H.z = cos_theta;

    // tangent space to world space
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bi_tangent = normalize(cross(N, tangent));

    vec3 sample_vec = tangent * H.x + bi_tangent * H.y + N * H.z;
    return normalize(sample_vec);
}

void main() {
    // we make approximation that view direction = surface normal to generate pre-filter.
    vec3 N = normalize(localPos);
    vec3 R = N;
    vec3 V = N;

    const uint SAMPLE_COUNT = 1024;
    float total_weight = 0.0;
    vec3 prefiltered_vec = vec3(0.0);
    for (uint i = 0; i < SAMPLE_COUNT; i++) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampling_GGX(Xi, N, roughness); // randomly sample possible half vector for surface
        vec3 L = normalize(2.0 * dot(V, H) * H - V); // calculate reflection from half vector and view direction to get light direction

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            prefiltered_vec += texture(cubemap, L).rgb * NdotL;
            total_weight += NdotL;
        }
    }

    prefiltered_vec = prefiltered_vec / total_weight;
    out_color = vec4(prefiltered_vec, 1.0);
}
