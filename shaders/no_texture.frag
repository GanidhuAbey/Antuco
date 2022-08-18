#version 450
#extension GL_EXT_debug_printf : enable

layout(location=0) out vec4 outColor;
layout(location=3) in vec3 surfaceNormal;
layout(location=4) in vec4 vPos;
layout(location=5) in vec2 texCoord;
layout(location=6) in vec4 light_perspective;

layout(location=7) in vec3 light_position;
layout(location=8) in vec3 light_color;
layout(location=9) in vec3 camera_pos;

layout(set=2, binding=1) uniform sampler2D shadowmap;

layout(set=3, binding=0) uniform Materials {
    vec3 has_texture;
    vec3 ambient; //Ka
    vec3 diffuse; //Kd
    vec3 specular; //Ks
} mat;

float bias = 5e-3;

int LIGHT_FACTOR = 1;
float SPECULAR_STRENGTH = 0.5f;
float FRESNEL_STRENGTH = 2.0f;
float AMBIENCE_FACTOR = 0.001f;
float DIFFUSE_STRENGTH = 0.5f;

//ok clearly we're not doing this right...
float SCATTER_STRENGTH = 200.0f;

//light perspective is now clamped from between the specified near plane and far plane
//going to hard code this values to check for now but will edit them once the effect
//is working as intended


//returns 0 if in shadow, otherwise 1
float check_shadow(vec4 light_view) {
	float pixel_depth = light_view.z;

  	//float closest_depth = texelFetch(shadowmap, ivec2(light_view.st), 0).r; 
    float closest_depth = texture(shadowmap, vec2(light_view.st), 0).r; 
  	
	float in_shadow = ceil(closest_depth - pixel_depth) + 0.1;

  	return in_shadow;
}


float pcf_shadow(vec4 light_view) {
    vec2 d = 1.0001 * 1 / textureSize(shadowmap, 0); //multiply by constant value, to make amplify softness of shadow

    //sample 4 points of the shadowmap
    float s1 = check_shadow(vec4(light_view.s + d.x, light_view.t, light_view.z, light_view.w));
    float s2 = check_shadow(vec4(light_view.s - d.x, light_view.t, light_view.z, light_view.w));
    float s3 = check_shadow(vec4(light_view.s, light_view.t + d.y, light_view.z, light_view.w));
    float s4 = check_shadow(vec4(light_view.s, light_view.t - d.y, light_view.z, light_view.w));

    float amb_val = 1.2f;
    return ((s1 + s2 + s3 + s4 + amb_val) / 5);
}



vec3 get_scattering(vec4 light_view) {
    float light_enter = texture(shadowmap, light_view.st).r;
    
    float light_exit = light_view.z;

    float surface_depth = light_exit - light_enter;

    return exp(-surface_depth * SCATTER_STRENGTH) * vec3(0.10f, 0.05f, 0.05f);
}

float map_to_zero_one(float value) {
    return min(ceil(max(0, value)), 1);
}


float rand() {
    vec3 x = vec3(vPos.x, vPos.y, vPos.z);
    return normalize(fract(sin(dot(x, vec3(12.9898, 78.233, 43.2003)))*43758.5453123)); 
}

float pi() {
    return 3.14159;
}

void main() {
    float dist = length(light_position - vec3(vPos));

    //this code treats a directional light as a point light...
    vec3 lightToObject = (light_position - vec3(vPos));
    //when the dot product should be at its highest, it seems to be at its lowest, and vice versa.
    float diffuse = 1 / pi();
    vec3 diffuse_final = diffuse * normalize(mat.diffuse);
  
    vec3 camera_dir = normalize(camera_pos - vec3(vPos));
    vec3 light_dir = normalize(light_position - vec3(vPos));
    vec3 m = (light_dir + camera_dir) / length(light_dir + camera_dir);

    //implement microfacet specular highlights
    vec3 surface_normal = normalize(surfaceNormal);
    float rough = pow(mat.specular.r, 2);

    float alignment = dot(surface_normal, m);
    float x = map_to_zero_one(alignment);
    float b = pow(alignment, 4);

    float a = (pow(alignment, 2) - 1)/(pow(rough, 2)*pow(alignment, 2));
    float v = exp(a);
   
    float r_squared = pow(rough, 2);
    float D_m = x*r_squared / (pi()*pow(1 + pow(alignment, 2)*(r_squared - 1), 2));

    //compute G_2
    float camera_alignment = dot(m, camera_dir);
    float light_alignment = dot(m, light_dir);
    float x_c = map_to_zero_one(camera_alignment);
    float x_l = map_to_zero_one(light_alignment);


    float macro_cam = pow(dot(surface_normal, camera_dir), 2);
    float macro_lig = pow(dot(surface_normal, light_dir), 2);

    float a_c =  macro_cam / (rough*(1-macro_cam));
    float a_l =  macro_lig / (rough*(1-macro_lig)); 

    float v_c = (-1 + sqrt(1 + 1/a_c))/2;
    float v_l = (-1 + sqrt(1 + 1/a_l))/2;
    
    float G_2 = (x_c*x_l)/(1+v_c+v_l);

    //compute F
    vec3 F = () + (1 - mat.diffuse)*pow(1 - max(0, dot(surface_normal, light_dir)), 5);

    //compute specular
    float bottom = 4*abs(dot(surface_normal, light_dir))*abs(dot(surface_normal, camera_dir));
    
    vec3 spec = (F*G_2*D_m)/bottom;

    //analyze depth at the given coordinate of the object 
    float light_dist = length(light_position - vec3(vPos));
    
    vec4 sample_value = light_perspective;

    float shadow_factor = pcf_shadow(sample_value);

    vec3 scattering = get_scattering(sample_value);
    
    //debugPrintfEXT("<%f, %f, %f> \n", surface_normal.x, surface_normal.y, surface_normal.z);
    float diffuse_component = mat.specular.r;
    vec3 result;
    result = vec3(0.05) + 
             max(vec3(0), vec3(1) * 
             dot(surface_normal, light_dir) * 
             ( diffuse_component * diffuse_final + 
               (1 - diffuse_component) * spec )  );

    //result = surfaceNormal;
    outColor = vec4(result, 1.0f);
}
