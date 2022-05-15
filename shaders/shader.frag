#version 450

layout(location=0) out vec4 outColor;
layout(location=3) in vec3 surfaceNormal;
layout(location=4) in vec4 vPos;
layout(location=5) in vec2 texCoord;
layout(location=6) in vec4 light_perspective;

layout(location=7) in vec3 light_position;
layout(location=8) in vec3 light_color;
layout(location=9) in vec3 camera_pos;

layout(set=2, binding=0) uniform sampler2D texture1;
layout(set=3, binding=1) uniform sampler2D shadowmap;

layout(set=4, binding=0) uniform Materials {
    vec3 has_texture;
    vec3 ambient; //Ka
    vec3 diffuse; //Kd
    vec3 specular; //Ks
} mat;

in vec4 gl_FragCoord;

float bias = 5e-3;

int LIGHT_FACTOR = 10;
float SPECULAR_STRENGTH = 1.0f;
float AMBIENCE_FACTOR = 0.01f;

//light perspective is now clamped from between the specified near plane and far plane
//going to hard code this values to check for now but will edit them once the effect
//is working as intended


//returns 0 if in shadow, otherwise 1
float check_shadow(vec3 light_view) {
	//light_view -= surfaceNormal * bias;
	float pixel_depth = light_view.z - bias;
  	//check if pixel_depth can sample closest depth
  	//then closest depth must be a valid value, so now we need to clamp to 0 or 1

  	float closest_depth = texture(shadowmap, light_view.st).r; 
  	
	float in_shadow = ceil(closest_depth - pixel_depth);
  	return in_shadow;
}

void main() {

    if (mat.has_texture.g != 1) {
        discard;
    }

    float dist = length(light_position - vec3(vPos));


    //get vector of light
    vec3 texture_component = vec3(texture(texture1, texCoord));

    vec3 lightToObject = normalize(light_position - vec3(vPos));
    float diffuse_light = max(0.f, dot(lightToObject, surfaceNormal)) * LIGHT_FACTOR;

    vec3 diffuse_final = diffuse_light * mat.diffuse;
    //float mapIntensity = (lightIntensity/2) + 0.5;

    //now sample the depth at that screen coordinate
    
    //specular lighting algorithm
    // 1 - take light from source to object, and reflect is through normal vector
    // 2 - take dot product between reflected vector and normal vector (clamp negatives to 0)
    // 3 - add value to final colour
    
    vec3 reflected_light = reflect(lightToObject, surfaceNormal);
    //need to pass data on the location of the camera
    vec3 object_to_camera = normalize(camera_pos - vec3(vPos));
    float specular_value = pow(max(0.f, dot(reflected_light, object_to_camera)), 32);
    vec3 specular_light = specular_value * mat.specular;

    //analyze depth at the given coordinate of the object
    float light_dist = length(light_position - vec3(vPos));

    //check z_buffer depth at this pixel locationS
    //the actual problem now is that im not sampling the texture correctly
    
    vec4 sample_value = light_perspective * (1/light_perspective.w);
    //float closest_to_light = (texture(shadowmap, sample_value.st)).r;

    //just solve it first, and then solve it well
    float shadow_factor = check_shadow(vec3(sample_value));
    
    //lets add diffuse light back into the equation
    //if texture is always 1, then no impact on outcome of img
    
    //TODO: get rid of this if statement once everything works
    vec3 result;
    if (mat.has_texture.r == 0) {
        texture_component = vec3(1);
    }

    
    result = (mat.ambient * AMBIENCE_FACTOR + diffuse_final + specular_light) * texture_component * shadow_factor;

    outColor = vec4(result, 0);
}
