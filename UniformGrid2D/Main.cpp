#include <windows.h>

//When using this as a template, be sure to make these changes in the new project: 
//1. In Debugging properties set the Environment to PATH=%PATH%;$(SolutionDir)\lib;
//2. Change window_title below
//3. Copy assets (mesh and texture) to new project directory

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files
#include "VideoMux.h"      //Functions for saving videos
#include "DebugCallback.h"
#include "BlitFbo.h"
#include "Bloom.h"
#include "UniformGrid2D.h"
#include "UniformGridGpu2D.h"
#include "UniformGridGpu3D.h"
#include "UniformGridParticles3D.h"
#include "ParallelScan.h"


const int init_window_width = 720;
const int init_window_height = 720;
const char* const window_title = "UniformGrid2D";

static const std::string vertex_shader("template_vs.glsl");
static const std::string fragment_shader("template_fs.glsl");
GLuint shader_program = -1;

RenderFbo render_fbo;
Bloom1 bloom;
BlitFbo blit_fbo;

//GridTestSelect grid_test(100);
//GridTestAnimate grid_test(2500);
//GridTestPbd grid_test(1024);
//GridTestGpuAnimate grid_test(2048); //must be a power of 2
//GridTestPbdGpu2D grid_test(1024*32);
GridTestPbdGpu3D grid_test(4*1024);

const int num_particles = 0;
//ParticleSystem particles(4*1024);
ParticleCollision particles(16*1024);

float auto_rotate_speed = 0.0f;
float angle = 0.0f;
float cam_dist = 7.0f;
float cam_height = 3.0f;
float target_height = 0.0f;
float scale = 1.0f;
float aspect = 1.0f;
bool recording = false;

//This structure mirrors the uniform block declared in the shader
struct SceneUniforms
{
    glm::mat4 M;
    glm::mat4 PV;	//camera projection * view matrix
    glm::vec4 eye_w;	//world-space eye 
} SceneData;

struct LightUniforms
{
    glm::vec4 La = glm::vec4(0.45f, 0.5f, 0.55f, 1.0f);	//ambient light color
    glm::vec4 Ld = glm::vec4(0.75f, 0.5f, 0.25f, 1.0f);	//diffuse light color
    glm::vec4 Ls = glm::vec4(0.3f);	//specular light color
    glm::vec4 light_w = glm::vec4(0.0f, 0.0, 0.0f, 1.0f); //world-space light position

} LightData;

struct MaterialUniforms
{
    glm::vec4 ka = glm::vec4(1.0f);	//ambient material color
    glm::vec4 kd = glm::vec4(1.0f);	//diffuse material color
    glm::vec4 ks = glm::vec4(1.0f);	//specular material color
    float shininess = 20.0f;         //specular exponent
} MaterialData;

//IDs for the buffer objects holding the uniform block data
GLuint scene_ubo = -1;
GLuint light_ubo = -1;
GLuint material_ubo = -1;

namespace UboBinding
{
    //These values come from the binding value specified in the shader block layout
    int scene = 0;
    int light = 1;
    int material = 2;
}

//Locations for the uniforms which are not in uniform blocks
namespace UniformLocs
{
    int M = 0; //model matrix
    int time = 1;
}

void draw_gui(GLFWwindow* window)
{
    //Begin ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //Draw Gui
    ImGui::Begin("Debug window");
    if (ImGui::Button("Quit"))
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    const int filename_len = 256;
    static char video_filename[filename_len] = "capture.mp4";

    ImGui::InputText("Video filename", video_filename, filename_len);
    ImGui::SameLine();
    if (recording == false)
    {
        if (ImGui::Button("Start Recording"))
        {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            recording = true;
            start_encoding(video_filename, w, h); //Uses ffmpeg
        }
    }
    else
    {
        if (ImGui::Button("Stop Recording"))
        {
            recording = false;
            finish_encoding(); //Uses ffmpeg
        }
    }
    ImGui::SliderFloat("Auto rotate speed", &auto_rotate_speed, -0.2, +0.2);
    ImGui::SliderFloat("View angle", &angle, -glm::pi<float>(), +glm::pi<float>());
    ImGui::SliderFloat("Cam dist", &cam_dist, 0.0f, 10.0f);
    ImGui::SliderFloat("Cam height", &cam_height, -2.0f, +2.0f);
    ImGui::SliderFloat("Target height", &target_height, -2.0f, +2.0f);

    ImGui::SliderFloat("Scale", &scale, -10.0f, +10.0f);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    //static bool show_test = false;
    //ImGui::ShowDemoWindow(&show_test);

    Module::sDrawGuiAll();

    //End ImGui Frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
    //Clear the screen to the color previously specified in the glClearColor(...) call.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SceneData.eye_w = glm::vec4(0.0f, cam_height, cam_dist, 1.0f);
    glm::mat4 M = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 V = glm::lookAt(glm::vec3(SceneData.eye_w), glm::vec3(0.0f, target_height, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 P = glm::perspective(glm::pi<float>() / 4.0f, aspect, 0.1f, 100.0f);
    SceneData.PV = P * V;
    SceneData.M = M;

    glUseProgram(shader_program);

    //Note that we don't need to set the value of a uniform here. The value is set with the "binding" in the layout qualifier

    //Set uniforms
    //glUniformMatrix4fv(UniformLocs::M, 1, false, glm::value_ptr(M));

    glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo); //Bind the OpenGL UBO before we update the data.
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneData), &SceneData); //Upload the new uniform values.
    glBindBuffer(GL_UNIFORM_BUFFER, 0); //unbind the ubo

    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::scene, scene_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::light, light_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::material, material_ubo);

    render_fbo.PreRender();
    grid_test.Draw();
    if (num_particles > 0)
    {
        particles.Draw();
    }
    render_fbo.PostRender();
    bloom.ComputeBloom();
    blit_fbo.Blit();

    glViewport(0, 0, init_window_width, init_window_height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    if (recording == true)
    {
        glFinish();
        glReadBuffer(GL_BACK);
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        read_frame_to_encode(&rgb, &pixels, w, h);
        encode_frame(rgb);
    }

    draw_gui(window);

    /* Swap front and back buffers */
    glfwSwapBuffers(window);
}

void idle()
{
    angle += auto_rotate_speed;
    float time_sec = static_cast<float>(glfwGetTime());

    //Pass time_sec value to the shaders
    glProgramUniform1f(shader_program, UniformLocs::time, time_sec);

    static float t = 0.0f;
    const float dt = 0.01f;
    t += dt;
    grid_test.Animate(t, dt);
    if (num_particles > 0)
    {
        particles.Animate(t, dt);
    }
}

void reload_shader()
{
    GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

    if (new_shader == -1) // loading failed
    {
        glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
    }
    else
    {
        glClearColor(0.15f, 0.15f, 0.15f, 0.0f);

        if (shader_program != -1)
        {
            glDeleteProgram(shader_program);
        }
        shader_program = new_shader;
    }
}

//This function gets called when a key is pressed
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //std::cout << "key : " << key << ", " << char(key) << ", scancode: " << scancode << ", action: " << action << ", mods: " << mods << std::endl;

    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case 'r':
        case 'R':
            reload_shader();
            break;

        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }

    Module::sKeyboardAll(key, scancode, action, mods);
}

//This function gets called when the mouse moves over the window.
void mouse_cursor(GLFWwindow* window, double x, double y)
{
    //std::cout << "cursor pos: " << x << ", " << y << std::endl; 

    glm::vec2 pos(x / init_window_width, y / init_window_height);
    pos = 2.0f * pos - glm::vec2(1.0f);
    pos.y *= -1.0f;

    Module::sMouseCursorAll(pos);
}

//This function gets called when a mouse button is pressed.
void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    //std::cout << "button : "<< button << ", action: " << action << ", mods: " << mods << std::endl;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec2 pos(x / init_window_width, y / init_window_height);
    pos = 2.0f * pos - glm::vec2(1.0f);
    pos.y *= -1.0f;

    Module::sMouseButtonAll(button, action, mods, pos);
}

void resize(GLFWwindow* window, int width, int height)
{
    //Set viewport to cover entire framebuffer
    glViewport(0, 0, width, height);
    //Set aspect ratio used in view matrix calculation
    aspect = float(width) / float(height);
}

//Initialize OpenGL state. This function only gets called once.
void initOpenGL()
{
    glewInit();

#ifdef _DEBUG
    RegisterCallback();
#endif

    //Print out information about the OpenGL version supported by the graphics driver.	
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    glEnable(GL_DEPTH_TEST);

    reload_shader();

    //Create and initialize uniform buffers
    glGenBuffers(1, &scene_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneUniforms), nullptr, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::scene, scene_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.

    glGenBuffers(1, &light_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, light_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUniforms), &LightData, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::light, light_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.

    glGenBuffers(1, &material_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, material_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialUniforms), &MaterialData, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
    glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::material, material_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if (num_particles > 0)
    {
        particles.pGrid = &grid_test.mGrid;
    }
    //ParallelScanTest();
    grid_test.Init();
    if (num_particles > 0)
    {
        particles.Init();
    }

    glm::ivec2 window_size(init_window_width, init_window_height);
    render_fbo.Init(window_size);
    bloom.Init(window_size);
    blit_fbo.Init(window_size, window_size);

    //Bypass bloom
    //blit_fbo.SetOutputTexture(render_fbo.mOutputTexture);

    //Bloom in pipeline
    bloom.SetInputTexture(render_fbo.mOutputTexture);
    blit_fbo.SetInputTexture(bloom.mOutputTex, 0);
}

int main(int argc, char** argv)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
    {
        return -1;
    }

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(init_window_width, init_window_height, window_title, NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    //Register callback functions with glfw. 
    glfwSetKeyCallback(window, keyboard);
    glfwSetCursorPosCallback(window, mouse_cursor);
    glfwSetMouseButtonCallback(window, mouse_button);
    glfwSetFramebufferSizeCallback(window, resize);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    initOpenGL();

    //Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        idle();
        display(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}