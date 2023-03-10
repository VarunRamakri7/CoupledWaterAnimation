#version 440

layout(location = 1) uniform float time;
layout(location = 2) uniform int pass;

//layout(binding = 0) uniform sampler2D wave_tex;
layout(binding = 1) uniform samplerCube skybox_tex;
layout(binding = 2) uniform sampler2D fbo_tex;
layout(binding = 3) uniform sampler2D depth_tex;

layout(std140, binding = 0) uniform SceneUniforms
{
    mat4 PV;
    mat4 P; // Projection Matrix
    mat4 V; // View matrix
    vec4 eye_w; // Camera eye in world-space
};

in VertexData
{
	vec3 particle_pos;
	vec2 tex_coord;
	float depth;
} inData;

out layout(location = 0) vec4 frag_color; //the output color for this fragment    

const vec4 particle_col = vec4(0.4f, 0.7f, 1.0f, 1.0f); // Light blue
const vec4 foam_col = vec4(1.0f); // White

const vec3 light_col = vec3(1.0f, 0.85f, 0.7f); // Warm light
const vec3 light_pos = vec3(1.0f, 1.0f, 0.0f); // Light position

const float near = 0.1f; // Near plane distance
const float far = 100.0f; // Far plane distance

vec3 WorldPosFromDepth(float depth);
vec4 blur();
float LinearizeDepth(float depth);
vec4 reflection();
vec4 refraction();
vec4 lighting();

void main ()
{    
    // Render particles
    if(pass == 0)
    {
        // Make circular particles
        float r = length(gl_PointCoord - vec2(0.5f));
        if (r >= 0.5f) discard;
	
	    // Add specular highlight
	    float spec = pow(max(dot(normalize(inData.particle_pos), normalize(light_pos)), 0.0f), 16.0f);
	    float a = exp(-12.0f * r) + 0.1f * exp(-2.0f * r); // Change transparency based on distance from center

        // Calculate lighting and skybox color
        vec4 refraction_color = refraction();
        vec4 reflection_color = reflection();
        vec4 lighting_color = lighting();
    
        vec4 combine = 0.75f * (reflection_color + refraction_color) + 0.1f * lighting_color;

	    // Change particle color depending on height
	    frag_color = mix(combine, foam_col, 2.0f * inData.particle_pos.y);
	    frag_color.rgb += spec; // Add specular highlight
	    frag_color.a = mix(0.1f, a, 2.0f * inData.particle_pos.y);

        //frag_color = lighting_color;
        //frag_color.a = 1.0f;
    }

    // Render full-screen quad
    if(pass == 1)
    {
        frag_color = texelFetch(fbo_tex, ivec2(gl_FragCoord), 0);
        //frag_color = texelFetch(depth_tex, ivec2(gl_FragCoord), 0);
        //frag_color = blur();
    }

    // Render particle depth
    if (pass == 2)
    {
        // Make circular particles
        float r = length(gl_PointCoord - vec2(0.5f));
        if (r >= 0.5f) discard;

        //float depth = LinearizeDepth(inData.depth) / far;
        frag_color = vec4(vec3(inData.depth), 1.0f);
    }
}

// Calculate normal from depth
vec3 GetNormalFromDepth()
{
    // Blur depth tetxure
    vec4 blur_frag = blur();

    // Use partial differences to calculate normal from depth
    float dzdx = texelFetch(depth_tex, ivec2(gl_FragCoord) + ivec2(1, 0), 0).x - 
                 texelFetch(depth_tex, ivec2(gl_FragCoord) + ivec2(-1, 0), 0).x;
    dzdx *= 0.5f;
    float dzdy = texelFetch(depth_tex, ivec2(gl_FragCoord) + ivec2(0, 1), 0).x - 
                 texelFetch(depth_tex, ivec2(gl_FragCoord) + ivec2(0, -1), 0).x;
    dzdy *= 0.5f;
    vec3 d = vec3(-dzdx, -dzdy, 1.0f);

    return normalize(d);
}

vec3 WorldPosFromDepth(float depth)
{
    float z = depth * 2.0f - 1.0f;

    vec4 clipSpacePosition = vec4(vec2(gl_FragCoord) * 2.0f - 1.0f, z, 1.0f);
    vec4 viewSpacePosition = inverse(P) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = inverse(V) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec4 blur()
{      
    int hw = 5;
    float n = 0.0f;

    vec4 blur = vec4(0.0);
    for(int i =- hw; i <= hw; i++)
    {
       for(int j =- hw; j <= hw; j++)
       {
          blur += texelFetch(depth_tex, ivec2(gl_FragCoord) + ivec2(i, j), 0);
          n += 1.0f;
       }
    }

    blur = blur / n;
    return blur;
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0f - 1.0f; // back to NDC 
    return (2.0f * near * far) / (far + near - z * (far - near));	
}

// From LearnOpenGL: https://learnopengl.com/Advanced-OpenGL/Cubemaps
vec4 reflection()
{
    vec3 normal = NormalFromDepth();
    float depth = texelFetch(depth_tex, ivec2(gl_FragCoord), 0).x;
    //vec3 pos = WorldPosFromDepth(depth);
    vec3 pos = inData.particle_pos;

    vec3 I = normalize(eye_w.xyz - pos);
    vec3 R = reflect(I, normal);
    return vec4(texture(skybox_tex, R).rgb, 1.0f);
}

// From LearnOpenGL: https://learnopengl.com/Advanced-OpenGL/Cubemaps
vec4 refraction()
{
    vec3 normal = NormalFromDepth();
    float depth = texelFetch(depth_tex, ivec2(gl_FragCoord), 0).x;
    //vec3 pos = WorldPosFromDepth(depth);
    vec3 pos = inData.particle_pos;

    float ratio = 1.0f / 1.33f; // Refractive index of Air/Water
    vec3 I = normalize(eye_w.xyz - pos);
    vec3 R = refract(I, normal, ratio);
    return vec4(texture(skybox_tex, R).rgb, 1.0f);
}

// From LearnOpenGL: https://learnopengl.com/code_viewer_gh.php?code=src/2.lighting/2.1.basic_lighting_diffuse/2.1.basic_lighting.fs
vec4 lighting()
{
    vec3 normal = NormalFromDepth();
    float depth = texelFetch(depth_tex, ivec2(gl_FragCoord), 0).x;
    vec3 pos = WorldPosFromDepth(depth);
    //vec3 pos = inData.particle_pos;

    // ambient
    float ambientStrength = 0.8f;
    vec3 ambient = ambientStrength * light_col;
  	
    // diffuse 
    vec3 lightDir = normalize(light_pos - pos);
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * light_col;
    diffuse *= 0.05f;
    
    // specular
    float specularStrength = 10.0f;
    vec3 viewDir = normalize(eye_w.xyz - pos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    vec3 specular = specularStrength * spec * light_col;

    return vec4((ambient + diffuse + specular) * particle_col.xyz, 1.0f);
}