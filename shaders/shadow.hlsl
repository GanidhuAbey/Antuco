#include "structures.hlsl"

struct VSInput 
{
    float3 position  : SV_POSITION;
    float3 normal    : NORMAL;
    float3 tex_coord : TEXCOORD;
};

struct PSInput 
{
    float3 position : POSITION;
};

ConstantBuffer<UBO> view : register(b1, space0);

#ifdef VERTEX

VSOutput main(VSInput input) {
    float3 pos = view.proj * 
                 view.world_to_camera * 
                 view.model_to_world * 
                 float4(input.position, 1.0f); 

    VSOutput output;

    output.position = pos;

    return output;
}

#endif