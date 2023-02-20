#include "UniformGui.h"
#include "imgui.h"
#include <cmath>


//Helper functions
bool CheckboxN(const char* label, bool* b, int n);
std::string ArrayElementName(const std::string& name, int index);
bool IsColor(const std::string& name);

void UniformGuiBasic(GLuint program)
{
   //Get the number of uniforms in program
   int num_uniforms = 0;
   glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

   ImGui::Begin("Uniforms"); 

   std::vector<GLchar> name(256);
   std::vector<GLenum> properties;
   //These are the properties of each uniform variable we are going to get the values of 
   properties.push_back(GL_NAME_LENGTH);
   properties.push_back(GL_TYPE);
   properties.push_back(GL_LOCATION);
   properties.push_back(GL_ARRAY_SIZE);

   //The values of the properties will be in here
   std::vector<GLint> values(properties.size());

   for(int uniform_i = 0; uniform_i < num_uniforms; ++uniform_i)
   {
      //get uniform variable properties
      glGetProgramResourceiv(program, GL_UNIFORM, uniform_i, properties.size(),
         &properties[0], values.size(), NULL, &values[0]);
 
      //get uniform variable name
      glGetProgramResourceName(program, GL_UNIFORM, uniform_i, name.size(), NULL, &name[0]);
   
      const int type = values[1];
      const int loc = values[2];
      if(type == GL_SAMPLER_2D)//type = 2d texture
      {
         ImGui::Text(name.data());
         GLint binding, tex_id;
         glGetUniformiv(program, loc, &binding);
         glGetIntegeri_v(GL_TEXTURE_BINDING_2D, binding, &tex_id);
         ImGui::Image((void*)tex_id, ImVec2(256,256));
      }

      /// 
      /// Floats
      /// 

      if(type == GL_FLOAT)
      {
         float v;
         //Get the current value of the uniform variable
         glGetUniformfv(program, loc, &v);
         //Make a slider to change the uniform variable
         if(ImGui::SliderFloat(name.data(), &v, 0.0f, 1.0f))
         {
            //If the slider was dragged, update the value of the uniform
            glProgramUniform1f(program, loc, v);
         }
      }
      if(type == GL_FLOAT_VEC2)
      {
         glm::vec2 v;
         glGetUniformfv(program, loc, &v.x);
         if(ImGui::SliderFloat2(name.data(), &v.x, 0.0f, 1.0f))
         {
            glProgramUniform2fv(program, loc, 1, &v.x);
         }
      }
      if(type == GL_FLOAT_VEC3)
      {
         glm::vec3 v;
         glGetUniformfv(program, loc, &v.x);
         if(ImGui::SliderFloat3(name.data(), &v.x, 0.0f, 1.0f))
         {
            glProgramUniform3fv(program, loc, 1, &v.x);
         }
      }
      if(type == GL_FLOAT_VEC4)
      {
         glm::vec4 v;
         glGetUniformfv(program, loc, &v.x);
         if(ImGui::SliderFloat4(name.data(), &v.x, 0.0f, 1.0f))
         {
            glProgramUniform4fv(program, loc, 1, &v.x);
         }
      }

      /// 
      /// Int
      /// 

      if(type == GL_INT)
      {
         int v;
         glGetUniformiv(program, loc, &v);
         if(ImGui::SliderInt(name.data(), &v, 0.0f, 1.0f))
         {
            glProgramUniform1i(program, loc, v);
         }
      }
      if(type == GL_INT_VEC2)
      {
         glm::ivec2 v;
         glGetUniformiv(program, loc, &v.x);
         if(ImGui::SliderInt2(name.data(), &v.x, 0.0f, 1.0f))
         {
            glProgramUniform2iv(program, loc, 1, &v.x);
         }
      }
      if(type == GL_INT_VEC3)
      {
         glm::ivec3 v;
         glGetUniformiv(program, loc, &v.x);
         if(ImGui::SliderInt3(name.data(), &v.x, 0.0f, 1.0f))
         {
            glProgramUniform3iv(program, loc, 1, &v.x);
         }
      }
      if(type == GL_INT_VEC4)
      {
         glm::ivec4 v;
         glGetUniformiv(program, loc, &v.x);
         if(ImGui::SliderInt4(name.data(), &v.x, 0.0f, 1.0f))
         {
            glProgramUniform4iv(program, loc, 1, &v.x);
         }
      }

      /// 
      /// Bool
      /// 

      if(type == GL_BOOL)
      {
         int v;
         glGetUniformiv(program, loc, &v);
         bool bv = v;
         if(ImGui::Checkbox(name.data(), &bv))
         {
            v = bv;
            glProgramUniform1i(program, loc, v);
         }
      }
      if(type == GL_BOOL_VEC2)
      {
         glm::ivec2 v;
         glGetUniformiv(program, loc, &v.x);
         glm::bvec2 bv(v);
         if(CheckboxN(name.data(), &bv.x, 2))
         {
            v = bv;
            glProgramUniform2iv(program, loc, 1, &v.x);
         }
      }
      if(type == GL_BOOL_VEC3)
      {
         glm::ivec3 v;
         glGetUniformiv(program, loc, &v.x);
         glm::bvec3 bv(v);
         if(CheckboxN(name.data(), &bv.x, 3))
         {
            v = bv;
            glProgramUniform3iv(program, loc, 1, &v.x);
         }
      }
      if(type == GL_BOOL_VEC4)
      {
         glm::ivec4 v;
         glGetUniformiv(program, loc, &v.x);
         glm::bvec4 bv(v);
         if(CheckboxN(name.data(), &bv.x, 4))
         {
            v = bv;
            glProgramUniform4iv(program, loc, 1, &v.x);
         }
      } 
   }
   ImGui::End();
}

void UniformGui(GLuint program)
{
   const int MAX_NAME_LEN = 256;
   int num_uniforms = 0;
   glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

   std::string title("UniformGui: Program ");
   title += std::to_string(program);
   
   if(ImGui::Begin(title.c_str()))
   { 
      static glm::vec2 float_range(0.0f, 1.0f);
      static glm::ivec2 int_range(0, 255);
      if (ImGui::CollapsingHeader("Settings"))
      {
         ImGui::InputFloat2("Float range", &float_range[0]);
         ImGui::InputInt2("Int range", &int_range[0]);
      }

      std::string name;
      name.resize(MAX_NAME_LEN);
      std::vector<GLenum> properties;
      properties.push_back(GL_NAME_LENGTH);
      properties.push_back(GL_TYPE);
      properties.push_back(GL_LOCATION);
      properties.push_back(GL_ARRAY_SIZE);

      std::vector<GLint> values(properties.size());

      for(int uniform_i = 0; uniform_i < num_uniforms; ++uniform_i)
      {
         //get uniform name
         glGetProgramResourceiv(program, GL_UNIFORM, uniform_i, properties.size(),
            &properties[0], values.size(), NULL, &values[0]);
 
         glGetProgramResourceName(program, GL_UNIFORM, uniform_i, name.size(), NULL, &name[0]);
   
         const int type = values[1];
         const int loc = values[2];
         const int array_size = values[3];

         if(loc == -1) continue; //If loc == -1 it is a variable from a uniform block

         if(type == GL_SAMPLER_2D)//type = 2d texture
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               ImGui::Text(element_name.data());
               GLint binding, tex_id;
               glGetUniformiv(program, loc+i, &binding);
               glGetIntegeri_v(GL_TEXTURE_BINDING_2D, binding, &tex_id);
               ImGui::Image((void*)tex_id, ImVec2(256,256));
            }
         }

         /// 
         /// Floats
         /// 

         if(type == GL_FLOAT)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               float v;
               glGetUniformfv(program, loc+i, &v);
               if(ImGui::SliderFloat(name.data(), &v, float_range[0], float_range[1]))
               {
                  glProgramUniform1f(program, loc+i, v);
               }
            }
         }
         if(type == GL_FLOAT_VEC2)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::vec2 v;
               glGetUniformfv(program, loc+i, &v.x);
               if(ImGui::SliderFloat2(element_name.data(), &v.x, float_range[0], float_range[1]))
               {
                  glProgramUniform2fv(program, loc+i, 1, &v.x);
               }
            }
         }
         if(type == GL_FLOAT_VEC3)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::vec3 v;
               glGetUniformfv(program, loc+i, &v.x);

               if(IsColor(element_name))
               {
                  if(ImGui::ColorEdit3(element_name.data(), &v.x))
                  {
                     glProgramUniform3fv(program, loc+i, 1, &v.x);
                  }
               }
               else
               {
                  if(ImGui::SliderFloat3(element_name.data(), &v.x, float_range[0], float_range[1]))
                  {
                     glProgramUniform3fv(program, loc+i, 1, &v.x);
                  }
               }
            }
         }
         if(type == GL_FLOAT_VEC4)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::vec4 v;
               glGetUniformfv(program, loc+i, &v.x);

               if(IsColor(element_name))
               {
                  if(ImGui::ColorEdit4(element_name.data(), &v.x))
                  {
                     glProgramUniform4fv(program, loc+i, 1, &v.x);
                  }
               }
               else
               {
                  if(ImGui::SliderFloat4(element_name.data(), &v.x, float_range[0], float_range[1]))
                  {
                     glProgramUniform4fv(program, loc+i, 1, &v.x);
                  }
               }
            }
         }

         if(type == GL_FLOAT_MAT4)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               element_name.resize(strlen(element_name.c_str()));
               glm::mat4 M;
               glGetUniformfv(program, loc+i, &M[0].x);
               for(int c=0; c<4; c++)
               {
                  std::string column_name = element_name + "[" + std::to_string(c) + "]";
                  ImGui::PushID(c);
                  if(ImGui::SliderFloat4(column_name.data(), &M[c].x, float_range[0], float_range[1]))
                  {
                     glProgramUniform4fv(program, loc+i, 1, &M[c].x);
                  }
                  ImGui::PopID();
               }
            }  
         }

         /// 
         /// Int
         /// 

         if(type == GL_INT)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               int v;
               glGetUniformiv(program, loc+i, &v);
               if(ImGui::SliderInt(element_name.data(), &v, int_range[0], int_range[1]))
               {
                  glProgramUniform1i(program, loc+i, v);
               }
            }
         }
         if(type == GL_INT_VEC2)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::ivec2 v;
               glGetUniformiv(program, loc+i, &v.x);
               if(ImGui::SliderInt2(element_name.data(), &v.x, int_range[0], int_range[1]))
               {
                  glProgramUniform2iv(program, loc+i, 1, &v.x);
               }
            }
         }
         if(type == GL_INT_VEC3)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::ivec3 v;
               glGetUniformiv(program, loc+i, &v.x);
               if(ImGui::SliderInt3(element_name.data(), &v.x, int_range[0], int_range[1]))
               {
                  glProgramUniform3iv(program, loc+i, 1, &v.x);
               }
            }
         }
         if(type == GL_INT_VEC4)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::ivec4 v;
               glGetUniformiv(program, loc+i, &v.x);
               if(ImGui::SliderInt4(element_name.data(), &v.x, int_range[0], int_range[1]))
               {
                  glProgramUniform4iv(program, loc+i, 1, &v.x);
               }
            }
         }

         /// 
         /// Bool
         /// 

         if(type == GL_BOOL)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               int v;
               glGetUniformiv(program, loc+i, &v);
               bool bv = v;
               if(ImGui::Checkbox(element_name.data(), &bv))
               {
                  v = bv;
                  glProgramUniform1i(program, loc+i, v);
               }
            }
         }
         if(type == GL_BOOL_VEC2)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::ivec2 v;
               glGetUniformiv(program, loc+i, &v.x);
               glm::bvec2 bv(v);
               if(CheckboxN(element_name.data(), &bv.x, 2))
               {
                  v = bv;
                  glProgramUniform2iv(program, loc+i, 1, &v.x);
               }
            }
         }
         if(type == GL_BOOL_VEC3)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::ivec3 v;
               glGetUniformiv(program, loc+i, &v.x);
               glm::bvec3 bv(v);
               if(CheckboxN(element_name.data(), &bv.x, 3))
               {
                  v = bv;
                  glProgramUniform3iv(program, loc+i, 1, &v.x);
               }
            }
         }
         if(type == GL_BOOL_VEC4)
         {
            for(int i=0; i<array_size; i++)
            {
               std::string element_name = ArrayElementName(name, i);
               glm::ivec4 v;
               glGetUniformiv(program, loc+i, &v.x);
               glm::bvec4 bv(v);
               if(CheckboxN(element_name.data(), &bv.x, 4))
               {
                  v = bv;
                  glProgramUniform4iv(program, loc+i, 1, &v.x);
               }
            }
         } 
      }
   }
   ImGui::End();
}

bool CheckboxN(const char* label, bool* b, int n)
{
   bool clicked = false;
   for(int i=0; i<n; i++)
   {
      ImGui::PushID(i);
      if(i<n-1)
      {
         if(ImGui::Checkbox("", &b[i]))
         {
            clicked = true;
         }
         ImGui::PopID();
         ImGui::SameLine();
      }
      else
      {
         if(ImGui::Checkbox(label, &b[i]))
         {
            clicked = true;
         }
         ImGui::PopID();
      }
   }
   return clicked;
}

std::string ArrayElementName(const std::string& name, int index)
{
   const std::string bracketed_zero("[0]");
   std::string element_name(name);
   size_t start_pos = name.find(bracketed_zero);
   if(start_pos == std::string::npos)
   {
      return element_name;
   }
   std::string index_str; index_str+='['; index_str+=std::to_string(index); index_str+=']';
   element_name.replace(start_pos, sizeof(bracketed_zero) - 1, index_str);
   return element_name;
}

inline bool ends_with(const std::string const & value, const std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool IsColor(const std::string& name)
{
   const std::string bracket("[");
   std::string element_name(name);
   size_t bracket_pos = name.find(bracket);
   element_name = element_name.substr(0, bracket_pos-1);
   element_name.resize(strlen(element_name.c_str()));

   if(ends_with(element_name, "rgb")) return true;
   if(ends_with(element_name, "rgba")) return true;
   return false;
}

UniformGuiContext::UniformGuiContext(GLuint program)
{
   Init(program);
}

void UniformGuiContext::Init(GLuint program)
{
   const int MAX_NAME_LEN = 256;
   mUniforms.clear();
   mNameMap.clear();
   mShader = program;

   int num_uniforms = 0;
   glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

   std::string name;
   name.resize(MAX_NAME_LEN);
   std::vector<GLenum> properties;
   properties.push_back(GL_NAME_LENGTH);
   properties.push_back(GL_TYPE);
   properties.push_back(GL_LOCATION);
   properties.push_back(GL_ARRAY_SIZE);

   std::vector<GLint> values(properties.size());

   for (int uniform_i = 0; uniform_i < num_uniforms; ++uniform_i)
   {
      //get uniform name
      glGetProgramResourceiv(program, GL_UNIFORM, uniform_i, properties.size(),
         &properties[0], values.size(), NULL, &values[0]);

      glGetProgramResourceName(program, GL_UNIFORM, uniform_i, name.size(), NULL, &name[0]);

      UniformVar u;
      u.mName = name;
      u.mName.resize(strlen(u.mName.data()));
      u.mType = values[1];
      u.mLoc = values[2];
      u.mArraySize = values[3];
      u.mValue.resize(u.mArraySize);

      if (u.mLoc == -1)
      {
         //This is a variable in a uniform block
         continue;
      }

      switch(u.mType)
      {
         case GL_FLOAT:
         case GL_FLOAT_VEC2:
         case GL_FLOAT_VEC3:
         case GL_FLOAT_VEC4:
            u.mFormat = "%.4f";
            for(int i=0; i< u.mArraySize; i++)
            {
               glm::vec4 val;
               glGetUniformfv(program, u.mLoc, &val.x);
               u.mValue[i] = val;
               if (val.x != 0.0f)
               {
                  int dec = glm::max(3, (int)glm::ceil(abs(log10(val.x))));
                  u.mFormat = "%."+std::to_string(dec+1)+"f";
               }
               
            }
         break;

         case GL_SAMPLER_2D:
         case GL_INT:
         case GL_INT_VEC2:
         case GL_INT_VEC3:
         case GL_INT_VEC4:
         case GL_BOOL:
         case GL_BOOL_VEC2:
         case GL_BOOL_VEC3:
         case GL_BOOL_VEC4:
            u.mFormat = "%.3d";
            for (int i = 0; i < u.mArraySize; i++)
            {
               glm::ivec4 val;
               glGetUniformiv(program, u.mLoc, &val.x);
               u.mValue[i] = val;
            }
         break;
      }

      mUniforms.push_back(u);
      mNameMap[u.mName] = mUniforms.size()-1;
   }

   //Find ranges
   for (UniformVar& u : mUniforms)
   {
      std::string range_name = u.mName + std::string("_range");

      std::map<std::string, int>::iterator it;
      it = mNameMap.find(range_name);
      if (it != mNameMap.end())
      {
         UniformVar& range_var = mUniforms[it->second];
         //TODO type checking
         //if ((p_range_var->mType == u.mType) &&
         //   (p_range_var->mArraySize == 2))
         if ((range_var.mType == GL_INT_VEC2)||
             (range_var.mType == GL_FLOAT_VEC2))
         {
            u.pRange = &range_var;
         }
      }
   }
}

void UniformGui(UniformGuiContext& context)
{
   std::string title("UniformGui: Program ");
   title += std::to_string(context.mShader);

   if (ImGui::Begin(title.c_str()))
   {
      for (UniformGuiContext::UniformVar& u : context.mUniforms)
      {
         if (u.mLoc == -1) continue; //If loc == -1 it is a variable from a uniform block
         
         float float_range[2] = {0.0f, 1.0f};
         int int_range[2] = { 0, 255};
         if (u.pRange != nullptr)
         {
            if(u.pRange->mType == GL_INT_VEC2)
            {
               int_range[0] = std::get<glm::ivec4>(u.pRange->mValue[0]).x;
               int_range[1] = std::get<glm::ivec4>(u.pRange->mValue[0]).y;
            }
            if (u.pRange->mType == GL_FLOAT_VEC2)
            {
               float_range[0] = std::get<glm::vec4>(u.pRange->mValue[0]).x;
               float_range[1] = std::get<glm::vec4>(u.pRange->mValue[0]).y;
            }
         }
         

         if (u.mType == GL_SAMPLER_2D)//type = 2d texture
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               ImGui::Text(element_name.data());
               GLint binding, tex_id;
               glGetUniformiv(context.mShader, u.mLoc + i, &binding);
               glGetIntegeri_v(GL_TEXTURE_BINDING_2D, binding, &tex_id);
               ImGui::Image((void*)tex_id, ImVec2(256, 256));
            }
         }

         /// 
         /// Floats
         /// 

         if (u.mType == GL_FLOAT)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::vec4& v = std::get<glm::vec4>(u.mValue[i]);
               if (ImGui::SliderFloat(u.mName.data(), &v.x, float_range[0], float_range[1], u.mFormat.c_str()))
               {
                  glProgramUniform1f(context.mShader, u.mLoc + i, v.x);
               }
            }
         }
         if (u.mType == GL_FLOAT_VEC2)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::vec4& v = std::get<glm::vec4>(u.mValue[i]);
               if (ImGui::SliderFloat2(element_name.data(), &v.x, float_range[0], float_range[1], u.mFormat.c_str()))
               {
                  glProgramUniform2fv(context.mShader, u.mLoc + i, 1, &v.x);
               }
            }
         }
         if (u.mType == GL_FLOAT_VEC3)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::vec4& v = std::get<glm::vec4>(u.mValue[i]);
               if (IsColor(element_name))
               {
                  if (ImGui::ColorEdit3(element_name.data(), &v.x))
                  {
                     glProgramUniform3fv(context.mShader, u.mLoc + i, 1, &v.x);
                  }
               }
               else
               {
                  if (ImGui::SliderFloat3(element_name.data(), &v.x, float_range[0], float_range[1], u.mFormat.c_str()))
                  {
                     glProgramUniform3fv(context.mShader, u.mLoc + i, 1, &v.x);
                  }
               }
            }
         }
         if (u.mType == GL_FLOAT_VEC4)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::vec4& v = std::get<glm::vec4>(u.mValue[i]);
               if (IsColor(element_name))
               {
                  if (ImGui::ColorEdit4(element_name.data(), &std::get<1>(u.mValue[i]).x))
                  {
                     glProgramUniform4fv(context.mShader, u.mLoc + i, 1, &v.x);
                  }
               }
               else
               {
                  if (ImGui::SliderFloat4(element_name.data(), &std::get<1>(u.mValue[i]).x, float_range[0], float_range[1], u.mFormat.c_str()))
                  {
                     glProgramUniform4fv(context.mShader, u.mLoc + i, 1, &v.x);
                  }
               }
            }
         }

         if (u.mType == GL_FLOAT_MAT4)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               element_name.resize(strlen(element_name.c_str()));
               glm::mat4 M;
               glGetUniformfv(context.mShader, u.mLoc + i, &M[0].x);
               for (int c = 0; c < 4; c++)
               {
                  std::string column_name = element_name + "[" + std::to_string(c) + "]";
                  ImGui::PushID(c);
                  if (ImGui::SliderFloat4(column_name.data(), &M[c].x, float_range[0], float_range[1], u.mFormat.c_str()))
                  {
                     glProgramUniform4fv(context.mShader, u.mLoc + i, 1, &M[c].x);
                  }
                  ImGui::PopID();
               }
            }
         }

         /// 
         /// Int
         /// 

         if (u.mType == GL_INT)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::ivec4& v = std::get<glm::ivec4>(u.mValue[i]);
               if (ImGui::SliderInt(element_name.data(), &v.x, int_range[0], int_range[1], u.mFormat.c_str()))
               {
                  glProgramUniform1i(context.mShader, u.mLoc + i, v.x);
               }
            }
         }
         if (u.mType == GL_INT_VEC2)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::ivec4& v = std::get<glm::ivec4>(u.mValue[i]);
               if (ImGui::SliderInt2(element_name.data(), &v.x, int_range[0], int_range[1], u.mFormat.c_str()))
               {
                  glProgramUniform2iv(context.mShader, u.mLoc + i, 1, &v.x);
               }
            }
         }
         if (u.mType == GL_INT_VEC3)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::ivec4& v = std::get<glm::ivec4>(u.mValue[i]);
               if (ImGui::SliderInt3(element_name.data(), &v.x, int_range[0], int_range[1], u.mFormat.c_str()))
               {
                  glProgramUniform3iv(context.mShader, u.mLoc + i, 1, &v.x);
               }
            }
         }
         if (u.mType == GL_INT_VEC4)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::ivec4& v = std::get<glm::ivec4>(u.mValue[i]);
               if (ImGui::SliderInt4(element_name.data(), &v.x, int_range[0], int_range[1], u.mFormat.c_str()))
               {
                  glProgramUniform4iv(context.mShader, u.mLoc + i, 1, &v.x);
               }
            }
         }

         /// 
         /// Bool
         /// 

         if (u.mType == GL_BOOL)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::bvec4& b = std::get<glm::bvec4>(u.mValue[i]);
               if (ImGui::Checkbox(element_name.data(), &b.x))
               {
                  glProgramUniform1i(context.mShader, u.mLoc + i, b.x);
               }
            }
         }
         if (u.mType == GL_BOOL_VEC2)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::bvec4& b = std::get<glm::bvec4>(u.mValue[i]);
               if (CheckboxN(element_name.data(), &b.x, 2))
               {
                  glProgramUniform2i(context.mShader, u.mLoc + i, b.x, b.y);
               }
            }
         }
         if (u.mType == GL_BOOL_VEC3)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::bvec4& b = std::get<glm::bvec4>(u.mValue[i]);
               if (CheckboxN(element_name.data(), &b.x, 3))
               {
                  glProgramUniform3i(context.mShader, u.mLoc + i, b.x, b.y, b.z);
               }
            }
         }
         if (u.mType == GL_BOOL_VEC4)
         {
            for (int i = 0; i < u.mArraySize; i++)
            {
               std::string element_name = ArrayElementName(u.mName, i);
               glm::bvec4& b = std::get<glm::bvec4>(u.mValue[i]);
               if (CheckboxN(element_name.data(), &b.x, 4))
               {
                  glProgramUniform4i(context.mShader, u.mLoc + i, b.x, b.y, b.z, b.w);
               }
            }
         }
      }
   }
   ImGui::End();
}
