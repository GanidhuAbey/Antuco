#include "structures.hlsl"

Texture2D     ShadowMap : register(t2, space1);
SamplerState  ShadowMapSampler : register(s1, space1);

struct Material {
  float3 has_texture;
  float3 ambient;
  float3 diffust;
  float3 specular;
};

ConstantBuffer<Material> mat   : register(b3, space0);
ConstantBuffer<UBO> view       : register(b1, space0);
ConstantBuffer<UBO> light_view : register(b0, space0)

[[vk::push_constant]]
cbuffer pfc {
  float3 light_color;
  float3 light_direction;
  float3 light_position;
  float3 light_count;
  float3 camera_pos;
}

struct VSInput {
  float3 position : POSITION0;
  float3 normal : NORMAL0;
  float2 tex_coord : TEXCOORD0;
}

struct PSInput {
  float4  position  : SV_POSITION;
  float3  normal    : NORMAL1;
  float2  tex_coord  : TEXCOORD1;

  // light  
  float4  light_pov_position : POSITION1;

  // world
  float4 world_position : POSITION2;
};

struct PSOutput {
  float4  oColor : SV_TARGET;
};

#define AMBIENT 0.1
#define PI 3.14159

#ifdef VERTEX
PSInput main(VSInput input) 
{
  PSInput result;

  result.position = view.proj * view.world_to_camera * view.model_to_world * float4(input.position, 1.f);
  result.normal = float3(ubo.model_to_world * float4(input.normal, 1.0f));
  result.light_pov_position = light_view.proj * light_view.world_to_camera * light_view.model_to_world * float4(input.position, 1.f);
  result.tex_coord = input.tex_coord;

  result.world_position = float3(ubo.model_to_world * float4(input.position, 1.0f));

  return result;
}

#endif

bool shadow(float2 shadowPos, float depth) 
{
  float sampleDepth = ShadowMap.Sample(ShadowMapSampler, shadowPos);

  bool in_shadow = step(sampleDepth, depth);

  return in_shadow;
}

float pcf_shadow(float2 shadowPos, float depth) 
{
  // TODO : pass dimensions through shader.
  uint width;
  uint height;
  ShadowMap.GetDimensions(width, height);

  float2 d = 1.f * rcp(float2(width, height))

  float s1 = shadow(shadowPos + float2(d.x, 0), depth);
  float s2 = shadow(shadowPos - float2(d.x, 0), depth);
  float s3 = shadow(shadowPos + float2(0, d.y), depth);
  float s4 = shadow(shadowPos - float2(0, d.y), depth);

  return ((s1+s2+s3+s4)/4);
}

#ifdef FRAGMENT

float3 compute_diffuse() 
{
  float diffuse_strength = 1 / PI;
  float3 diffuse_color = diffuse_strength * normalize(mat.diffuse);

  float3 diffuse_color; 
}

float3 compute_specular(LightingData light) 
{
  float3 normal = normalize(light.surface_normal);

  float roughness = pow(mat.specular.r, 2);
  
  float3 h = normalize(light.in_dir + light.out_dir);

  float microsurface_offset = dot(h, normal);
  float x = any(abs(microsurface_offset));
  float b = pow(microsurface_offset, 4);

  float a = (pow(microsurface_offset, 2) - 1)/(pow(roughness, 2)*pow(microsurface_offset, 2));
  float v = exp(a);

  float r_squared = pow(roughness, 2);
  float D_m = x*r_squared / (PI * pow(1 + pow(microsurface_offset, 2) * (r_squared  - 1), 2));

  float camera_alignment = dot(h, camera_dir);
  float light_alignment = dot(h, camera_dir);

  float x_c = any(abs(camera_alignment));
  float x_l = any(abs(light_alignment));

  float macro_cam = pow(dot(normal, light.out_dir), 2);
  float macro_lig = pow(dot(normal, light.in_dir), 2);

  float a_c = macro_cam / (roughness * (1 - marco_cam));
  float a_l = macro_lig / (roughness * (1 - macro_lig));

  float v_c = (-1 + sqrt(1 + 1/a_c))/2;
  float v_l = (-1 + sqrt(1 + 1/a_l))/2;
  
  float G_2 = (x_c*x_l)/(1+v_c+v_l);

  //compute F
  //need to linearly interpolate the mettalic value to computer f0
  float3 f0 = mix(float3(0.04), mat.diffuse, mat.specular.g);
  float3 F = f0 + (1 - f0)*pow(1 - max(0, dot(normal, light.in_dir)), 5);

  //compute specular
  float bottom = 4*abs(dot(normal, light.in_dir))*abs(dot(normal, light.out_dir));
 
  float3 spec = (F*G_2*D_m)/bottom;

  return spec;
}

PSOutput main(PSInput input)
{
  LightingData light;
  light.surface_normal = input.normal;
  light.in_dir = normalize(pfc.light_position - input.world_position);
  light.out_dir = normalize(pfc.camera_pos - input.world_position);

  // compute diffuse
  float3 diffuse = compute_diffuse();

  // compute specular
  float3 specular = compute_specular(light);

  float3 ambient = float3(AMBIENT);

  float3 result = ambient + 
                  dot(light.surface_normal, light.dir) * 
                  (0.5 * diffuse + 0.5 * spec);
  
  PSOutput output;
  output.oColor = result;

  return output;
}
#endif