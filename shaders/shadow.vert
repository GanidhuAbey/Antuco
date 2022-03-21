#version 450


//this ubo data should be relative to the light source(s)
//would have to run a render pass for each light source if we want to generate a shadow map for multiple light sources  
layout(binding = 1) uniform UniformBufferObject  {
	mat4 modelToWorld;
    mat4 worldToCamera;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;


void main() {
    gl_Position =  ubo.projection * ubo.worldToCamera * ubo.modelToWorld * vec4(inPosition, 1.0); //opengl automatically divids the components of the vector by 'w'
}
