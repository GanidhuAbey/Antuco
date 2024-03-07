#version 450

struct UniformBufferTemplate {
    mat4 modelToWorld;
    mat4 worldToEye;
    mat4 projection;
};

layout(set=1, binding = 0) uniform UniformBufferObject  {
    mat4 modelToWorld;
    mat4 worldToCamera;
    mat4 projection;
} ubo;

layout(set=0, binding = 1) uniform LightBufferObject {
	mat4 model_to_world;
    mat4 world_to_light;
    mat4 projection;
} lbo;

layout(push_constant) uniform PushFragConstant {
  vec3 lightColor;
  vec3 lightDirection;
  vec3 lightPosition;
  vec3 light_count;
  vec3 camera;
} pfc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

//TODO: actually bring these in, probably through a push constant if we can


layout(location = 3) out vec3 surfaceNormal;
layout(location = 4) out vec4 vPos;
layout(location = 5) out vec2 texCoord;
layout(location = 6) out vec4 light_perspective;

//light data into fragment shader
layout(location = 7) out vec3 light_position;
layout(location = 8) out vec3 light_color;

layout(location = 9) out vec3 camera_pos;

//now i know the size length
float mapping_value = (1/sqrt(pfc.light_count.x))*0.5;

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

    vPos = ubo.modelToWorld * vec4(inPosition, 1.0);
    light_perspective = (/*biasMat */ lbo.projection * lbo.world_to_light * lbo.model_to_world) * vec4(inPosition, 1.0);
    texCoord = inTexCoord;
    light_perspective.xyz = light_perspective.xyz / light_perspective.w;
    light_perspective.xy = (light_perspective.xy + 1) / 2;

    light_position = pfc.lightPosition;
    light_color = pfc.lightColor;
    camera_pos = pfc.camera;
}
