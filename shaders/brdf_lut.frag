#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(location=1) in vec2 uv;
layout(location=0) out vec2 out_color;

const float PI = 3.14159265359;

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

float GGX_G(vec3 v, vec3 n, float roughness) {
    float NdotV = max(dot(n, v), 0.0);
    float k = (roughness * roughness) / 2.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float Smith_G(vec3 l, vec3 v, vec3 n, float roughness) {
    return GGX_G(l, n, roughness) * GGX_G(v, n, roughness);
}

vec2 IntegrateBRDF(float NdotV, float roughness) {
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampling_GGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = Smith_G(L, V, N, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main() {
    out_color = IntegrateBRDF(uv.x, uv.y);
}
