struct View

struct PerDraw 
{
    float4x4 modelToClipMatrix;
};

ConstantBuffer<PerDraw>  draw_data : register(b0, space0) 

struct VSInput 
{
    float4 position;
    float3 normal;
    float2 tex_coord;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex_coord : TEXCOORD0;
};

struct PSOutput 
{
    float4 o_color : SV_TARGET;
};

VSOutput vsmain(VSInput input)
{
    VSOutput out;

    out.position = mul(draw_data.modelToClipMatrix, position);
    out.normal = 
}
