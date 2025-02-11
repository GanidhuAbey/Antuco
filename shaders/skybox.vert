#version 450

struct UniformBufferTemplate {
    mat4 modelToWorld;
    mat4 worldToEye;
    mat4 projection;
};

layout(set=0, binding = 1) uniform UniformBufferObject  {
    mat4 modelToWorld;
    mat4 worldToCamera;
    mat4 projection;
} ubo;

layout(push_constant) uniform PushFragConstant {
  vec3 camera;
} pfc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

//TODO: actually bring these in, probably through a push constant if we can

layout(location = 4) out vec3 camera_pos;
layout(location = 3) out vec4 vPos;
layout(location = 2) out vec3 surfaceNormal;
layout(location = 1) out vec2 texCoord;
//layout(location = 6) out vec4 light_perspective;


void main() {

    //matrix in row-major
    mat4 biasMat = mat4(
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.5, 0.5, 0.0, 1.0
    );


    gl_Position = ubo.projection * ubo.worldToCamera * ubo.modelToWorld * vec4(inPosition, 1.0); //opengl automatically divids the components of the vector by 'w'
    // TODO: likely faster to computer inverse CPU side.
    surfaceNormal = normalize(vec3(transpose(inverse(ubo.modelToWorld)) * vec4(inNormal, 0.0)));
    texCoord = inTexCoord;
    vPos = ubo.modelToWorld * vec4(inPosition, 1.0);
    camera_pos = pfc.camera;
}
