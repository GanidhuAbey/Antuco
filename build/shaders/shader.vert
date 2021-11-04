#version 450

layout(set=0, binding = 0) uniform UniformBufferObject  {
    mat4 modelToWorld;
    mat4 worldToCamera;
    mat4 projection;
} ubo;

layout(set=1, binding = 1) uniform LightBufferObject {
    mat4 model_to_world;
    mat4 world_to_light;
    mat4 projection;
} lbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 3) out vec3 surfaceNormal;
layout(location = 4) out vec4 vPos;
layout(location = 5) out vec2 texCoord;
layout(location = 6) out vec4 light_perspective;

//will hard code this value just to see if it works...
vec3 light_pos = vec3(0.0, 5.0, 0.0);

mat4 mapping_matrix = mat4(
    0.5, 0, 0, 0.5,
    0, 0.5, 0, 0.5,
    0, 0, 0.5, 0.5,
    0, 0, 0, 1
);


const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.5,
	0.0, 0.5, 0.0, 0.5,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0 );

void main() {

    gl_Position =  ubo.projection * ubo.worldToCamera * ubo.modelToWorld * vec4(inPosition, 1.0); //opengl automatically divids the components of the vector by 'w'

    surfaceNormal = vec3(ubo.modelToWorld * vec4(inNormal, 1.0));
    vPos = ubo.modelToWorld * vec4(inPosition, 1.0);
    light_perspective = biasMat * lbo.projection * lbo.world_to_light * lbo.model_to_world * vec4(inPosition, 1.0);
    texCoord = inTexCoord;
}
