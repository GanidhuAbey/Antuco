#version 450
#extension GL_EXT_debug_printf : enable

#define PI 3.1415926535897932384626433832795
#define EPSILON 0.001

layout(location=0) out vec4 outColor;
layout(location=3) in vec3 surfaceNormal;
layout(location=4) in vec4 vPos;
layout(location=5) in vec2 texCoord;
//layout(location=6) in vec4 light_perspective;

layout(location=7) in vec3 light_position;
layout(location=8) in vec3 light_color;
layout(location=9) in vec3 camera_pos;

//layout(set=2, binding=1) uniform sampler2D shadowmap;

layout(set=1, binding=0) uniform Material {
    vec3 pbrParameters; // baseReflectivity, roughness, metallic
    vec3 hasTexture; // hasDiffuse, hasMetallic, hasRoughness
    vec3 albedo;

    vec4 padding[2];
} mat;

layout(set=1, binding=1) uniform sampler2D diffuseTexture;
layout(set=1, binding=2) uniform sampler2D roughnessMetallicTexture;
layout(set=1, binding=3) uniform sampler2D metallicTexture; // used if roughness and metallic are separate.

// set 1 = draw, set = 2 material, set = 3 pass, set = 4 scene

layout(set=2, binding=0) uniform samplerCube irradianceMap;
layout(set=2, binding=1) uniform samplerCube specularIblMap;
layout(set=2, binding=2) uniform sampler2D brdf_map;

float bias = 5e-3;

int LIGHT_FACTOR = 1;
float SPECULAR_STRENGTH = 0.5f;
float FRESNEL_STRENGTH = 2.0f;
float AMBIENCE_FACTOR = 0.001f;
float DIFFUSE_STRENGTH = 0.5f;

const float MAX_REFLECTION_LOD = 4.0;

//ok clearly we're not doing this right...
float SCATTER_STRENGTH = 200.0f;

//light perspective is now clamped from between the specified near plane and far plane
//going to hard code this values to check for now but will edit them once the effect
//is working as intended


////returns 0 if in shadow, otherwise 1
//float check_shadow(vec4 light_view) {
//	float pixel_depth = light_view.z;
//
//  	//float closest_depth = texelFetch(shadowmap, ivec2(light_view.st), 0).r;
//    float closest_depth = texture(shadowmap, vec2(light_view.st), 0).r;
//
//	float in_shadow = ceil(closest_depth - pixel_depth) + 0.1;
//
//  	return in_shadow;
//}
//
//
//float pcf_shadow(vec4 light_view) {
//    vec2 d = 1.0001 * 1 / textureSize(shadowmap, 0); //multiply by constant value, to make amplify softness of shadow
//
//    //sample 4 points of the shadowmap
//    float s1 = check_shadow(vec4(light_view.s + d.x, light_view.t, light_view.z, light_view.w));
//    float s2 = check_shadow(vec4(light_view.s - d.x, light_view.t, light_view.z, light_view.w));
//    float s3 = check_shadow(vec4(light_view.s, light_view.t + d.y, light_view.z, light_view.w));
//    float s4 = check_shadow(vec4(light_view.s, light_view.t - d.y, light_view.z, light_view.w));
//
//    float amb_val = 1.2f;
//    return ((s1 + s2 + s3 + s4 + amb_val) / 5);
//}
//
//
//
//vec3 get_scattering(vec4 light_view) {
//    float light_enter = texture(shadowmap, light_view.st).r;
//
//    float light_exit = light_view.z;
//
//    float surface_depth = light_exit - light_enter;
//
//    return exp(-surface_depth * SCATTER_STRENGTH) * vec3(0.10f, 0.05f, 0.05f);
//}

float map_to_zero_one(float value) {
    return min(ceil(max(0, value)), 1);
}


float rand() {
    vec3 x = vec3(vPos.x, vPos.y, vPos.z);
    return normalize(fract(sin(dot(x, vec3(12.9898, 78.233, 43.2003)))*43758.5453123));
}

float getAlphaSquared(float roughness) {
    return pow(roughness, 4);
}

vec3 getHalfVector(vec3 v, vec3 l) {
    return normalize(l + v);
}

vec3 Schlick_F(vec3 h, vec3 v, vec3 baseReflect, float roughness) {
    return baseReflect + (max(vec3(1.0 - roughness), baseReflect) - baseReflect)*pow(clamp(1.0 - dot(h, v), 0.0, 1.0), 5.0);
}

float GGX_G(vec3 v, vec3 n, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float NdotV = max(dot(n, v), 0.0);

    float num = NdotV;
    float denom = NdotV * (1.0 - k) * k;

    return num / denom;
}

float Smith_G(vec3 l, vec3 v, vec3 n, float roughness) {
    return GGX_G(l, n, roughness) * GGX_G(v, n, roughness);
}

float GGX_D(vec3 n, vec3 h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(n, h), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denominator = NdotH2*(a2 - 1.0) + 1.0;
    return a2 / (PI * denominator * denominator);
}

// Cook Torrence BRDF (Specular)
vec3 getSpecular(vec3 l, vec3 v, vec3 n, vec3 F, float roughness) {
    float alphaSquared = getAlphaSquared(roughness);
    vec3 h = getHalfVector(l, v);
    float D = GGX_D(n, h, alphaSquared);
    float G = Smith_G(l, v, n, alphaSquared);

    return (D*G*F) / ((4.0 * max(dot(l, n), 0.0) * max(dot(l, n), 0.0)) + 0.0001);
}

void main(){
    // TODO : introduce hard coded parameters as attributes that are controllable within the engine.
    // ---------- Hard coded paramaters
    vec3 lightColor = vec3(1.f, 1.f, 1.f); // colour of the incoming light [LIGHT]
    vec3 materialBaseReflectivity = vec3(mat.pbrParameters.x); // visually good enough for dieletric materials.

    // Weird artifacts on the roughness texture...
    float roughness = mat.hasTexture.z == 1.f ? texture(roughnessMetallicTexture, texCoord).y : mat.pbrParameters.y;
    //float metallic = mat.hasTexture.z == 1.f ? texture(roughnessMetallicTexture, texCoord).z : mat.pbrParameters.z;
    vec3 albedo = mat.hasTexture.x == 1.f ? texture(diffuseTexture, texCoord).xyz : mat.albedo; // surface color

    float metallic = mat.pbrParameters.z;
    if (mat.hasTexture.y == 1.0) {
        metallic = texture(metallicTexture, texCoord).x;
    }
    else if (mat.hasTexture.z == 1.0) {
        metallic = texture(roughnessMetallicTexture, texCoord).z;    
    }

    materialBaseReflectivity = mix(materialBaseReflectivity, albedo, metallic);

    vec3 lightDirection = normalize(light_position - vec3(vPos));
    vec3 viewDirection = normalize(camera_pos - vec3(vPos));
    vec3 h = getHalfVector(lightDirection, viewDirection);

    vec3 irradiance = texture(irradianceMap, surfaceNormal).rgb;

    // ---------- Diffuse ---------------
    vec3 kS = Schlick_F(surfaceNormal, viewDirection, materialBaseReflectivity, roughness);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 ambientDiffuse = irradiance * albedo;

    vec3 diffuse = albedo;

    // ---------- Specular --------------
    float D = GGX_D(viewDirection, surfaceNormal, roughness);
    float G = Smith_G(lightDirection, viewDirection, surfaceNormal, roughness);
    vec3 F = Schlick_F(h, viewDirection, materialBaseReflectivity, roughness);

    float NdotV = max(dot(viewDirection, surfaceNormal), 0.0);
    float NdotL = max(dot(lightDirection, surfaceNormal), 0.0);

    vec3 specular = (D * G * F) / ((4.0 * NdotV) * (4.0 * NdotL) + 0.0001);

    vec3 R = reflect(-viewDirection, surfaceNormal);
    vec3 prefiliteredColor = textureLod(specularIblMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(brdf_map, vec2(max(dot(surfaceNormal, viewDirection), 0.0), roughness)).rg;

    vec3 ambientSpecular = prefiliteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * ambientDiffuse + ambientSpecular);

    // cook torrence specular: (D(h) * F * G(v, h)) / (4 * dot(n*v) * dot(n*l))

    // reflectance equation : L_o = integral(f_r*L_o*cos(theta) dw_i)

    float attenuation = 1.0f; // TODO : implement light falloff.
    vec3 radiance = lightColor * attenuation;
    vec3 Lo = (kD * diffuse / PI + specular) * radiance * NdotL;


    vec3 result = ambient + Lo;
//
//    // HDR tonemapping
//    result = result / (result + vec3(1.0));
//    // gamma correct
//    result = pow(result, vec3(1.0/2.2)); 

    outColor = vec4(result, 1.0f);
}
