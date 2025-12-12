#version 450
#extension GL_EXT_debug_printf : enable

layout(location=0) out vec4 outColor;
layout(location=5) in mat3 surfaceNormal;
layout(location=8) in vec4 vPos;
layout(location=9) in vec2 texCoord;
//layout(location=6) in vec4 light_perspective;

layout(location=10) in vec3 light_position;
layout(location=11) in vec3 light_color;
layout(location=12) in vec3 camera_pos;

layout(set=1, binding=0) uniform sampler2D texture1;
//layout(set=3, binding=1) uniform sampler2D shadowmap;

layout(set=2, binding=0) uniform Materials {
    vec3 has_texture;
    vec3 ambient; //Ka
    vec3 diffuse; //Kd
    vec3 specular; //Ks
} mat;

float bias = 5e-3;

int LIGHT_FACTOR = 1;
float SPECULAR_STRENGTH = 0.5f;
float AMBIENCE_FACTOR = 0.001f;
float DIFFUSE_STRENGTH = 0.5f;

//ok clearly we're not doing this right...
float SCATTER_STRENGTH = 200.0f;

//light perspective is now clamped from between the specified near plane and far plane
//going to hard code this values to check for now but will edit them once the effect
//is working as intended


//returns 0 if in shadow, otherwise 1
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

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.f);
}
