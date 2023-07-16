Texture2D OutputTexture : register(t2, space0);
SamplerState OutputSampler : register(s1, space0);

struct PSInput 
{
    float2 tex_coord : TEXCOORD;
};

struct VSInput 
{
    float vertex_index : SV_VertexID
};

struct PSOutput 
{
    o_color : SV_TARGET;
};

#ifdef VERTEX 

void main(VSInput input) 
{
    PSInput output;
    output.tex_coord = float2((input.vertex_index << 1) & 2, 
                               input.vertex_index & 2);

    return output;
}

#endif

#ifdef FRAGMENT

PSOutput main(PSInput input) 
{
    PSOutput output;
    
    float4 output_color = OutputTexture.Sample(OutputSampler, input.tex_coord);

    output.o_color = output_color;

    return output;
}

#endif