#include <windows.h>

//When using this as a template, be sure to make these changes in the new project: 
//1. In Debugging properties set the Environment to PATH=%PATH%;$(SolutionDir)\lib;
//2. Change window_title below
//3. Copy assets (mesh and texture) to new project directory

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>

#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files
#include "VideoMux.h"      //Functions for saving videos
#include "DebugCallback.h"
#include "AttriblessRendering.h"
#include "Bloom.h"
#include "Caustics.h"
#include "GridMesh.h"

int window_width = 1024;
int window_height = 1024;
const char* const window_title = "Wave Equation";

static const std::string vertex_shader("wave_vs.glsl");
static const std::string fragment_shader("wave_fs.glsl");
GLuint shader_program = -1;

static const std::string grid_vertex_shader("grid_vs.glsl");
static const std::string grid_fragment_shader("grid_fs.glsl");
GLuint grid_program = -1;

static const std::string grid_tess_vertex_shader("grid_tess_vs.glsl");
static const std::string grid_tess_control_shader("grid_tess_tc.glsl");
static const std::string grid_tess_eval_shader("grid_tess_te.glsl");
static const std::string grid_tess_fragment_shader("grid_tess_fs.glsl");
GLuint grid_tess_program = -1;

GLuint fbo = -1;
GLuint u_tex[3] = {-1, -1, -1};
int ix0 = 2; //index of u at current timestep, t. 
int ix1 = 1; //index of u at previous timestep, t-1.
int ix2 = 0; //index of u at earlier timestep, t-2.
GLuint image_tex = -1;

Bloom bloom;
Caustics caustics;
DrawElementsIndirect indirect_grid;

void ping_pong_buffers()
{
   ix0 = (ix0+1)%3;
   ix1 = (ix1+1)%3;
   ix2 = (ix2+1)%3;
}

GLuint attribless_vao = -1;

float aspect = 1.0f;
bool recording = false;
bool pause_wave = false;


//This structure mirrors the uniform block declared in the shader
struct SceneUniforms
{
   glm::mat4 PV;	//camera projection * view matrix

   glm::vec4 mouse_pos = glm::vec4(-1.0f, -1.0f, 20.0f, 0.0f); //x, y, brush radius
   glm::ivec4 mouse_buttons = glm::ivec4(0);
   glm::vec4 params[2] = {glm::vec4(0.2097f, 0.1050f, 1.0f, 1.0f), 
                           glm::vec4(0.54, 0.62f, 0.0f, 0.0f)};
   glm::vec4 pal[4] = {glm::vec4(1.0f, 0.5f, 0.5f, 1.0f), glm::vec4(0.5f), glm::vec4(2.0f), glm::vec4(0.0f)};
} SceneData;

//IDs for the buffer objects holding the uniform block data
GLuint scene_ubo = -1;

namespace UboBinding
{
   //These values come from the binding value specified in the shader block layout
   int scene = 0; 
}

//Locations for the uniforms which are not in uniform blocks
namespace UniformLocs
{
   int M = 0; //model matrix
   int time = 1;
   int pass = 2;
   int iteration = 3;
   int lambda = 4;
   int atten = 5;
   int beta = 6;
   int source = 7;
   int obstacle = 8;
   int brush = 9;
}

void init_textures();


//For an explanation of this program's structure see https://www.glfw.org/docs/3.3/quick.html 

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

   if(ImGui::Button("Clear"))
   {
      init_textures();
   }
   ImGui::SameLine();
   if(ImGui::Button("Pause"))
   {
      pause_wave = !pause_wave;
   }

   static float lambda = 0.2f*0.2f;
   if(ImGui::SliderFloat("lambda", &lambda, 0.0f, 1.0f))
   {
      glProgramUniform1f(shader_program, UniformLocs::lambda, lambda);
   }

   static float atten = 0.9995f;
   if(ImGui::SliderFloat("atten", &atten, 0.9f, 1.0f, "%0.4f"))
   {
      glProgramUniform1f(shader_program, UniformLocs::atten, atten);
   }

   static float beta = 0.001f;
   if(ImGui::SliderFloat("beta", &beta, 0.0f, 0.1f, "%0.4f"))
   {
      glProgramUniform1f(shader_program, UniformLocs::beta, beta);
   }

   static int source = 0;
   if(ImGui::SliderInt("source", &source, 0, 2))
   {
      glProgramUniform1i(shader_program, UniformLocs::source, source);
   }

   static int obstacle = 0;
   if(ImGui::SliderInt("obstacle", &obstacle, 0, 2))
   {
      glProgramUniform1i(shader_program, UniformLocs::obstacle, obstacle);
   }

   static int brush = 0;
   if(ImGui::SliderInt("brush", &brush, 0, 2))
   {
      glProgramUniform1i(shader_program, UniformLocs::brush, brush);
   }

   ImGui::SliderFloat("Brush radius", &SceneData.mouse_pos.z, 0.8f, 100.0f);
   ImGui::SliderFloat4("Params0", &SceneData.params[0].x, 0.0f, 1.0f);
   ImGui::SliderFloat4("Params1", &SceneData.params[1].x, 0.0f, 1.0f);
   ImGui::ColorEdit3("Pal0", &SceneData.pal[0].x);
   ImGui::ColorEdit3("Pal1", &SceneData.pal[1].x);
   ImGui::ColorEdit3("Pal2", &SceneData.pal[2].x);
   ImGui::ColorEdit3("Pal3", &SceneData.pal[3].x);

   static glm::vec4 slider(0.0f);
   if(ImGui::SliderFloat4("slider", &slider.x, 0.0f, 1.0f, "%0.6f"))
   {
      glProgramUniform4fv(grid_tess_program, 2, 1, &slider.x);
   }
   

   //Show fbo_tex for debugging purposes. This is highly recommended for multipass rendering.
   float size = 128.0f;
   //ImGui::Image((void*)u_tex[ix0], ImVec2(size, size), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
   //ImGui::Image((void*)u_tex[ix1], ImVec2(size, size), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0)); 
   //ImGui::Image((void*)u_tex[ix2], ImVec2(size, size), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0)); 
   ImGui::Image((void*)image_tex, ImVec2(size, size), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0)); 

   ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
   ImGui::End();

   //static bool show_test = false;
   //ImGui::ShowDemoWindow(&show_test);
   caustics.DrawGui();

   //End ImGui Frame
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void display_2d(GLFWwindow* window);
void display_3d(GLFWwindow* window);

// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
   //display_2d(window);
   display_3d(window);     
}


void display_2d(GLFWwindow* window)
{
   glUseProgram(shader_program);

   glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo); //Bind the OpenGL UBO before we update the data.
   glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneData), &SceneData); //Upload the new uniform values.
   glBindBuffer(GL_UNIFORM_BUFFER, 0); //unbind the ubo

   for(int i=0; i<10; i++)
   {
      //Pass 0: write ix0, read ix1, ix2
      glUniform1i(UniformLocs::pass, 0);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to FBO.
      glDrawBuffer(GL_COLOR_ATTACHMENT0+ix0);

      //Make the viewport match the FBO texture size.
      glViewport(0, 0, window_width, window_height);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


      glBindTextureUnit(1, u_tex[ix1]);
      glBindTextureUnit(2, u_tex[ix2]);
      glBindVertexArray(attribless_vao);
      draw_attribless_quad();

      ping_pong_buffers();
   }

   //Pass 1
   glUniform1i(UniformLocs::pass, 1);
   glDrawBuffer(GL_COLOR_ATTACHMENT3);
   
   //Make the viewport match the FBO texture size.
   glViewport(0, 0, window_width, window_height);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
   glBindTextureUnit(0, u_tex[ix0]);
   glBindVertexArray(attribless_vao);
   draw_attribless_quad();

   //Blit caustics image
   caustics.Render(u_tex[ix0]);
   //glBindFramebuffer(GL_READ_FRAMEBUFFER, caustics.fbo);
   //glReadBuffer(GL_COLOR_ATTACHMENT0);

   //Blit bloom image
   bloom.ComputeBloom(caustics.output_tex);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, bloom.fbo);
   glReadBuffer(GL_COLOR_ATTACHMENT3);

   //Blit image tex
   //glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
   //glReadBuffer(GL_COLOR_ATTACHMENT3);

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
   glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);        
   
  
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

void display_3d(GLFWwindow* window)
{
   glm::vec4 eye_w = glm::vec4(0.0f, 0.25f, 0.75f, 1.0f);
   glm::mat4 V = glm::lookAt(glm::vec3(eye_w), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 P = glm::perspective(glm::pi<float>()/5.0f, aspect, 0.1f, 100.0f);
   SceneData.PV = P*V;

   glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo); //Bind the OpenGL UBO before we update the data.
   glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneData), &SceneData); //Upload the new uniform values.
   glBindBuffer(GL_UNIFORM_BUFFER, 0); //unbind the ubo
   
   glUseProgram(shader_program);

   if(pause_wave==false)
   {
   for(int i=0; i<10; i++)
   {
      //Pass 0: write ix0, read ix1, ix2
      glUniform1i(UniformLocs::pass, 0);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to FBO.
      glDrawBuffer(GL_COLOR_ATTACHMENT0+ix0);

      //Make the viewport match the FBO texture size.
      glViewport(0, 0, window_width, window_height);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


      glBindTextureUnit(1, u_tex[ix1]);
      glBindTextureUnit(2, u_tex[ix2]);
      glBindVertexArray(attribless_vao);
      draw_attribless_quad();

      ping_pong_buffers();
   }
   }
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glDrawBuffer(GL_BACK);

   //glUseProgram(grid_program);

   glUseProgram(grid_tess_program);
   indirect_grid.mode = GL_PATCHES;
   glPatchParameteri(GL_PATCH_VERTICES, 3);

   glm::mat4 M = glm::rotate(glm::pi<float>()/2.0f, glm::vec3(1.0f, 0.0f, 0.0f))*glm::translate(glm::vec3(-0.5, -0.5, 0.0));
   //glm::mat4 M = glm::scale(glm::vec3(0.35f))*glm::translate(glm::vec3(-0.5, -0.5, 0.0));
   //Set uniforms
   glUniformMatrix4fv(UniformLocs::M, 1, false, glm::value_ptr(M));

   glViewport(0, 0, window_width, window_height);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
   glBindTextureUnit(0, u_tex[ix0]);
   glBindTextureUnit(1, u_tex[ix1]);
   glBindTextureUnit(2, u_tex[ix2]);

   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   indirect_grid.BindAllAndDraw(); 
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);     

  
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
//   float time_sec = static_cast<float>(glfwGetTime());
static float time_sec = 0.0f;
time_sec += 1.0f/60.0f;


   //Pass time_sec value to the shaders
   glProgramUniform1f(shader_program, UniformLocs::time, time_sec);

   static int iteration=0;
   glProgramUniform1i(shader_program, UniformLocs::iteration, iteration);
   iteration++;
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
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

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

   if(action == GLFW_PRESS)
   {
      switch(key)
      {
         case 'r':
         case 'R':
            reload_shader();
            caustics.ReloadShader();
         break;

         case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
         break;     
      }
   }
}

//This function gets called when the mouse moves over the window.
void mouse_cursor(GLFWwindow* window, double x, double y)
{
    //std::cout << "cursor pos: " << x << ", " << y << std::endl;
   SceneData.mouse_pos.x = x;
   SceneData.mouse_pos.y = window_height-y; //flip y coordinate
}

//This function gets called when a mouse button is pressed.
void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
   //std::cout << "button : "<< button << ", action: " << action << ", mods: " << mods << std::endl;
   
   ImGuiIO& io = ImGui::GetIO();
   if(io.WantCaptureMouse == false)
   {
      SceneData.mouse_buttons[button] = action;
   }
}

void resize(GLFWwindow* window, int width, int height)
{
   window_width = width;
   window_height = height;
   //Set viewport to cover entire framebuffer
   glViewport(0, 0, width, height);
   //Set aspect ratio used in view matrix calculation
   aspect = float(width)/float(height);
}

void init_textures()
{
   std::vector<glm::vec4> init(window_width*window_height);
   static std::default_random_engine gen;
   std::uniform_real_distribution<float> urand(0.0f, 1.0f);
   std::normal_distribution<float> nrand(0.0f, 0.01f);

   glm::vec2 cen(0.5f*window_width, 0.5f*window_height);
   int idx = 0;
   for(int i=0; i<window_width; i++)
   for(int j=0; j<window_height; j++)
   {
      init[idx] = glm::vec4(0.0f);

      //if(idx == window_width*window_height/2 + window_width/2) init[idx] = glm::vec4(0.1f);

      //glm::vec4 noise = glm::vec4(nrand(gen), nrand(gen), nrand(gen), nrand(gen));
      //init[idx] = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f) + noise;

      /*
      if(glm::distance(glm::vec2(i,j), cen) < 20.0f)
      {
         init[idx] = glm::vec4(0.5f, 0.25f, 0.5f, 0.25f);
      }
      */
      idx++;
   }

   for(int i=0; i<3; i++)
   {
      glBindTexture(GL_TEXTURE_2D, u_tex[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, init.data());
      glBindTexture(GL_TEXTURE_2D, 0);
   }
}

//Initialize OpenGL state. This function only gets called once.
void initOpenGL()
{
   glewInit();

   #ifdef _DEBUG
      RegisterCallback();
   #endif

   //Print out information about the OpenGL version supported by the graphics driver.	
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;

   reload_shader();

   glGenVertexArrays(1, &attribless_vao);

   glGenTextures(3, u_tex);
   for(int i=0; i<3; i++)
   {
      glBindTexture(GL_TEXTURE_2D, u_tex[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glBindTexture(GL_TEXTURE_2D, 0);
   }

   init_textures();

   glGenTextures(1, &image_tex);
   glBindTexture(GL_TEXTURE_2D, image_tex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);

   //Create the framebuffer object
   glGenFramebuffers(1, &fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);
   for(int i=0; i<3; i++)
   {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, u_tex[i], 0);
   }

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, image_tex, 0);

   //unbind the fbo
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   //Create and initialize uniform buffers
   glGenBuffers(1, &scene_ubo);
   glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneUniforms), nullptr, GL_STREAM_DRAW); //Allocate memory for the buffer, but don't copy (since pointer is null).
   glBindBufferBase(GL_UNIFORM_BUFFER, UboBinding::scene, scene_ubo); //Associate this uniform buffer with the uniform block in the shader that has the same binding.
   
   glBindBuffer(GL_UNIFORM_BUFFER, 0);

   bloom.Init(window_width, window_height);
   caustics.Init(window_width, window_height);


   //For surface vis
   indirect_grid = CreateIndirectGridMeshTriangles(20);
   grid_program = InitShader(grid_vertex_shader.c_str(), grid_fragment_shader.c_str());
   grid_tess_program = InitShader(grid_tess_vertex_shader.c_str(), grid_tess_control_shader.c_str(), grid_tess_eval_shader.c_str(), grid_tess_fragment_shader.c_str());
}

//C++ programs start executing in the main() function.
int main(int argc, char **argv)
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
   window = glfwCreateWindow(window_width, window_height, window_title, NULL, NULL);
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
   glfwSwapInterval(1);	


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