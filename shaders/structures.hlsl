struct UBO {
  float4x4 model_to_world;
  float4x4 world_to_camera;
  float4x4 proj;
};

struct LightingData {
  float3 in_dir;
  float3 out_dir;
  float3 surface_normal;
};