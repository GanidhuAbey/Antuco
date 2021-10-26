#version 450

layout(push_constant) uniform PushFragConstant {
  vec3 lightColor;
  vec3 lightPosition;
} pfc;

layout(location=0) out vec4 outColor;
layout(location=3) in vec3 surfaceNormal;
layout(location=4) in vec4 vPos;
layout(location=5) in vec2 texCoord;
layout(location=6) in vec4 light_perspective;

layout(set=2, binding=0) uniform sampler2D texture1;
layout(set=3, binding=1) uniform sampler2D shadowmap;

in vec4 gl_FragCoord;

//light perspective is now clamped from between the specified near plane and far plane
//going to hard code this values to check for now but will edit them once the effect
//is working as intended

float near = 1.0;
float far = 96.0;

void main() {
    //get vector of light
    vec3 lightToObject = normalize(pfc.lightPosition - vec3(vPos));

    float lightIntensity = max(0.01f, dot(lightToObject, surfaceNormal));
    //float mapIntensity = (lightIntensity/2) + 0.5;

    vec3 newColor = pfc.lightColor * lightIntensity;

    //now sample the depth at that screen coordinate

    //analyze depth at the given coordinate of the object
    float depth_from_light = (light_perspective.z / light_perspective.w);

    float light_dist = length(pfc.lightPosition - vec3(vPos));

    //check z_buffer depth at this pixel location
    //the actual problem now is that im not sampling the texture correctly
    
    vec4 sample_value = light_perspective * (1/light_perspective.w);
    float closest_to_light = (texture(shadowmap, vec2(sample_value))).r;

    //float fragment_depth = (1/(far-near))*(light_perspective.z - near);

    float shadow_factor = closest_to_light - sample_value.z;

    outColor = vec4(0.1, 0, sample_value.z, 1.0);
}
