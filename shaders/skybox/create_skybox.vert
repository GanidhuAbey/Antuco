#version 450
#extension GL_KHR_vulkan_glsl: enable

struct UniformBufferTemplate {
    mat4 modelToWorld;
    mat4 worldToEye;
    mat4 projection;
};

layout(set=0, binding = 0) uniform UniformBufferObject  {
    mat4 modelToWorld;
    mat4 worldToCamera;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;
// technically our model doesn't need this extra data
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 1) out vec3 localPosition;
layout(location = 2) out vec2 uv;


void main() {
	//uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	//gl_Position = vec4(uv * 2 - 1, 0.0f, 1.0f);
    
    localPosition = inPosition;
    uv = inTexCoord;

    mat4 projection = mat4(2.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 2.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 2.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);

    gl_Position = ubo.projection * ubo.worldToCamera * vec4(localPosition, 1.0);
}
