#version 450

layout(push_constant) uniform LightObject {
  vec3 lightColor;
  vec3 lightPosition;
} light;

layout(location=0) out vec4 outColor;
layout(location=3) in vec3 surfaceNormal;
layout(location=4) in vec4 vPos;
layout(location=5) in vec2 texCoord;

layout(set=1, binding=0) uniform sampler2D texture1;

in vec4 gl_FragCoord;

void main() {
    //get vector of light
    vec3 lightToObject = normalize(light.lightPosition - vec3(vPos));

    float lightIntensity = max(0.01f, dot(lightToObject, surfaceNormal));
    //float mapIntensity = (lightIntensity/2) + 0.5;

    vec3 newColor = lightIntensity * light.lightColor;

    outColor = vec4(newColor, 1.0) * texture(texture1, texCoord);
}
