#version 450
#extension GL_EXT_debug_printf : enable

#define PI 3.1415926535897932384626433832795

layout(location=0) out vec4 outColor;
layout(location=3) in vec3 surfaceNormal;
layout(location=4) in vec4 vPos;
layout(location=5) in vec2 texCoord;
layout(location=6) in vec4 light_perspective;

layout(location=7) in vec3 light_position;
layout(location=8) in vec3 light_color;
layout(location=9) in vec3 camera_pos;

layout(set=2, binding=1) uniform sampler2D shadowmap;

layout(set=3, binding=0) uniform Material {
    vec3 pbrParameters; // baseReflectivity, roughness, metallic
    vec3 albedo;

    vec4 padding[2];
} mat;

layout(set=3, binding=1) uniform sampler2D matDiffuse;

float bias = 5e-3;

int LIGHT_FACTOR = 1;
float SPECULAR_STRENGTH = 0.5f;
float FRESNEL_STRENGTH = 2.0f;
float AMBIENCE_FACTOR = 0.001f;
float DIFFUSE_STRENGTH = 0.5f;

//ok clearly we're not doing this right...
float SCATTER_STRENGTH = 200.0f;

//light perspective is now clamped from between the specified near plane and far plane
//going to hard code this values to check for now but will edit them once the effect
//is working as intended


//returns 0 if in shadow, otherwise 1
float check_shadow(vec4 light_view) {
	float pixel_depth = light_view.z;

  	//float closest_depth = texelFetch(shadowmap, ivec2(light_view.st), 0).r;
    float closest_depth = texture(shadowmap, vec2(light_view.st), 0).r;

	float in_shadow = ceil(closest_depth - pixel_depth) + 0.1;

  	return in_shadow;
}


float pcf_shadow(vec4 light_view) {
    vec2 d = 1.0001 * 1 / textureSize(shadowmap, 0); //multiply by constant value, to make amplify softness of shadow

    //sample 4 points of the shadowmap
    float s1 = check_shadow(vec4(light_view.s + d.x, light_view.t, light_view.z, light_view.w));
    float s2 = check_shadow(vec4(light_view.s - d.x, light_view.t, light_view.z, light_view.w));
    float s3 = check_shadow(vec4(light_view.s, light_view.t + d.y, light_view.z, light_view.w));
    float s4 = check_shadow(vec4(light_view.s, light_view.t - d.y, light_view.z, light_view.w));

    float amb_val = 1.2f;
    return ((s1 + s2 + s3 + s4 + amb_val) / 5);
}



vec3 get_scattering(vec4 light_view) {
    float light_enter = texture(shadowmap, light_view.st).r;

    float light_exit = light_view.z;

    float surface_depth = light_exit - light_enter;

    return exp(-surface_depth * SCATTER_STRENGTH) * vec3(0.10f, 0.05f, 0.05f);
}

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

vec3 Schlick_F(vec3 h, vec3 v, vec3 baseReflect) {
    return baseReflect + (1.0 - baseReflect)*pow(clamp(1.0 - dot(h, v), 0.0, 1.0), 5.0);
}

float GGX_G(vec3 v, vec3 n, float alphaSquared) {
    float NdotV = dot(n, v);
    float numerator = 2.0*NdotV;

    float denomTerm = alphaSquared + (1.0 - alphaSquared)*pow(NdotV, 2.0);
    float denominator = NdotV + sqrt(denomTerm);

    return numerator / denominator;
}

float Smith_G(vec3 l, vec3 v, vec3 n, float alphaSquared) {
    return GGX_G(l, n, alphaSquared) * GGX_G(v, n, alphaSquared);
}

float GGX_D(vec3 n, vec3 h, float alphaSquared) {
    float denominator = PI*pow(pow(dot(n, h), 2)*(alphaSquared - 1) + 1, 2);
    return alphaSquared / denominator;
}

// Cook Torrence BRDF (Specular)
vec3 getSpecular(vec3 l, vec3 v, vec3 n, vec3 F, float roughness) {
    float alphaSquared = getAlphaSquared(roughness);
    vec3 h = getHalfVector(l, v);
    float D = GGX_D(n, h, alphaSquared);
    float G = Smith_G(l, v, n, alphaSquared);

    return (D*G*F) / (4 * dot(l, n) * dot(v, n));
}

void main(){
    // TODO : introduce hard coded parameters as attributes that are controllable within the engine.
    // ---------- Hard coded paramaters
    vec3 lightColor = vec3(1.f, 1.f, 1.f); // colour of the incoming light [LIGHT]
    vec3 materialBaseReflectivity = vec3(mat.pbrParameters.x); // visually good enough for dieletric materials.
    float roughness = mat.pbrParameters.y;
    float metallic = mat.pbrParameters.z;
    vec3 albedo = mat.albedo; // surface color

    materialBaseReflectivity = mix(materialBaseReflectivity, albedo, metallic);

    vec3 lightDirection = normalize(light_position - vec3(vPos));
    vec3 viewDirection = normalize(camera_pos - vec3(vPos));
    vec3 h = getHalfVector(lightDirection, viewDirection);
    vec3 F = Schlick_F(h, viewDirection, materialBaseReflectivity);

    float reflectAmt = min(max(length(F), 0.0), 1.0);
    float refractAmt = 1 - reflectAmt;
    refractAmt *= 1.0 - metallic;

    // ---------- Diffuse ---------------
    vec3 diffuseResult = albedo / PI;

    // ---------- Specular --------------
    vec3 specularResult = getSpecular(lightDirection, viewDirection, surfaceNormal, F, roughness);

    // cook torrence specular: (D(h) * F * G(v, h)) / (4 * dot(n*v) * dot(n*l))

    // reflectance equation : L_o = integral(f_r*L_o*cos(theta) dw_i)

    vec3 result = lightColor * (refractAmt*diffuseResult + reflectAmt*specularResult)*dot(surfaceNormal, lightDirection);
    //result = surfaceNormal;
    vec3 testColor = vec3(1, 0, 0);
    outColor = vec4(result, 1.0f);
}
