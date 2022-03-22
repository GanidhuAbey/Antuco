#version 450

layout(location=0) out vec4 outColor;
layout(location=3) in vec3 surfaceNormal;
layout(location=4) in vec4 vPos;
layout(location=5) in vec2 texCoord;
layout(location=6) in vec4 light_perspective;

layout(location=7) in vec3 light_position;
layout(location=8) in vec3 light_color;

layout(set=2, binding=0) uniform sampler2D texture1;
layout(set=3, binding=1) uniform sampler2D shadowmap;

in vec4 gl_FragCoord;

float bias = 5e-3;

int LIGHT_FACTOR = 10;

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
    //get vector of light
    vec3 lightToObject = normalize(light_position - vec3(vPos));

    float lightIntensity = max(0.01f, dot(lightToObject, surfaceNormal)) * LIGHT_FACTOR;
    //float mapIntensity = (lightIntensity/2) + 0.5;

    //now sample the depth at that screen coordinate

    //analyze depth at the given coordinate of the object
    float light_dist = length(light_position - vec3(vPos));

    //check z_buffer depth at this pixel location
    //the actual problem now is that im not sampling the texture correctly
    
    vec4 sample_value = light_perspective * (1/light_perspective.w);
    //float closest_to_light = (texture(shadowmap, sample_value.st)).r;

    //just solve it first, and then solve it well
    float shadow_factor = check_shadow(vec3(sample_value));
    
    //lets add diffuse light back into the equation 
    outColor = vec4(vec3(texture(texture1, texCoord)) * shadow_factor * lightIntensity, 1.0);
}
