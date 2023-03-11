#version 440

layout(binding = 1) uniform sampler2D diffuse_tex;

layout(location = 1) uniform float time;

layout(std140, binding = 0) uniform SceneUniforms
{
	mat4 PV;
	mat4 P;
	mat4 V;
	vec4 eye_w; // Camera eye in world-space
};

in VertexData
{
	vec2 tex_coord;
	vec3 pos;
	vec3 normal;
} inData;

out vec4 fragcolor; //the output color for this fragment    

const vec3 boat_col = vec3(0.5f, 0.33f, 0.3f);
const vec3 light_col = vec3(1.0f, 0.85f, 0.7f); // Warm light
const vec3 light_pos = vec3(0.0f, 1.0f, 0.0f); // Light position

vec4 lighting();

void main(void)
{   
	//fragcolor = texture(diffuse_tex, inData.tex_coord);
	
    fragcolor = lighting();
}

// From LearnOpenGL: https://learnopengl.com/code_viewer_gh.php?code=src/2.lighting/2.1.basic_lighting_diffuse/2.1.basic_lighting.fs
vec4 lighting()
{
    // ambient
    float ambientStrength = 0.8f;
    vec3 ambient = ambientStrength * light_col;
  	
    // diffuse 
    vec3 norm = normalize(inData.normal);
    vec3 lightDir = normalize(light_pos - inData.pos);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = diff * light_col;
    diffuse *= 0.05f;
    
    // specular
    float specularStrength = 0.5f;
    vec3 viewDir = normalize(eye_w.xyz - inData.pos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    vec3 specular = specularStrength * spec * light_col;  
        
    return vec4((ambient + diffuse + specular) * boat_col, 1.0f);
}