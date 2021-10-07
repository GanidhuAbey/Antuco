#version 450

layout(set=0, binding = 0) uniform UniformBufferObject  {
    mat4 modelToWorld;
    mat4 worldToCamera;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 3) out vec3 surfaceNormal;
layout(location = 4) out vec4 vPos;
layout(location = 5) out vec2 texCoord;


void main() {

    gl_Position =  ubo.projection * ubo.worldToCamera * ubo.modelToWorld * vec4(inPosition, 1.0); //opengl automatically divids the components of the vector by 'w'

    surfaceNormal = vec3(ubo.modelToWorld * vec4(inNormal, 1.0));
    vPos = ubo.modelToWorld * vec4(inPosition, 1.0);
    texCoord = inTexCoord;
}
