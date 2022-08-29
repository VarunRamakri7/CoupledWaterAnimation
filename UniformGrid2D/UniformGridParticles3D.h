#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <GL/glew.h>
#include "Aabb.h"
#include "ParallelScan.h"
#include "Module.h"
#include "UniformGridGpu3D.h"


class ComputeShader
{
   public:
      std::string mFilename;
      GLuint mShader = -1; 
      glm::ivec3 mNumWorkgroups = glm::ivec3(0);
      glm::ivec3 mSize = glm::ivec3(0);

      ComputeShader(std::string filename);
      void Init();
      void SetSize(glm::ivec3 size);
      void Dispatch();
      glm::ivec3 GetMaxWorkGroup() {return mMaxWorkGroup;}

   private:
      glm::ivec3 mMaxWorkGroup = glm::ivec3(1024, 1, 1);
      
};

class Buffer
{
   public:
      GLuint mBuffer = -1;
      GLuint mBinding = -1;
      GLenum mTarget = -1;
      GLuint mFlags = -1;
      GLuint mSize = 0;
      bool mEnableDebug = false;
      
      Buffer(GLuint target, GLuint binding = -1);
      void Init(int size, void* data, GLuint flags);
      void BindBufferBase();
      void DebugReadFloat(); //to do: template
      void DebugReadInt();
};


class Ugrid3D
{
   public:
   ComputeShader mComputeShader = ComputeShader("ugrid_particles_cs.glsl");
   Buffer mGridInfoUbo = Buffer(GL_UNIFORM_BUFFER, 10);

   Buffer mZerosSsbo = Buffer(GL_SHADER_STORAGE_BUFFER);
   Buffer mGridCounterSsbo = Buffer(GL_SHADER_STORAGE_BUFFER, 10);
   Buffer mGridOffsetSsbo = Buffer(GL_SHADER_STORAGE_BUFFER, 11);

   struct UniformGridInfo
   {
      aabb3D mExtents;
      glm::ivec4 mNumCells;
      glm::vec4 mCellSize;

      int num_cells() {return mNumCells.x*mNumCells.y*mNumCells.z;}
   } mUniformGridInfo;

   struct Uniforms
   {
      const int COMPUTE_COUNTS = 0;
      const int COMPUTE_OFFSETS = 1;
      int mMode = COMPUTE_COUNTS;
   } mUniforms;

   struct UniformLoc
   {
      int mMode = 0;
   } mUniformLoc;

   ParallelScan mParallelScan;

   Ugrid3D(glm::ivec4 num_cells, aabb3D extents);

   void ClearCounter();
   void ClearOffset();
   void ComputeOffsets();
   void Init();
};

class UgridParticles3D : public Ugrid3D
{
   public:
 
      struct Particle
      {
         glm::vec4 p;
         glm::vec4 v;
      };

      Buffer mIndexListSsbo = Buffer(GL_SHADER_STORAGE_BUFFER, 12);
      Buffer mParticleSsbo = Buffer(GL_SHADER_STORAGE_BUFFER, 13);

      glm::ivec3 mParticleWorkgroups = glm::ivec3(0);

      struct Uniforms
      {
         //const int COMPUTE_COUNTS = 0;  //in Ugrid3D
         //const int COMPUTE_OFFSETS = 1; //in Ugrid3D
         const int INSERT_PARTICLES = 2;
         //const int PREANIMATE = 3;      //in physics
         const int COLLISION_QUERY = 4;
         //const int POSTANIMATE = 5;     //in physics

         int mNumParticles = 0;
      } mUniforms;

      struct UniformLoc
      {
         int mMode = 0;
         int mNumParticles = 1;
      } mUniformLoc;

      UgridParticles3D(glm::ivec4 num_cells, aabb3D extents);

      void Init(std::vector<Particle>& particles);
      void Init(GLuint particleSsbo, int num_particles);
      void BuildGrid();
      
};

class ParticlePhysics
{
   public:   
      ComputeShader mComputeShader = ComputeShader("ugrid_particle_physics_cs.glsl");

      //Physics parameters
      struct Params
      {
         glm::vec4 g = glm::vec4(0.0f, -0.1f, 0.0f, 0.0f); //gravity
         float dt = 0.02f;
         float damping = 0.999f;
         float e = 0.75f; //coefficient of restitution
         int n = 0; //num particles
      } mParams;

      Buffer mParamsUbo = Buffer(GL_UNIFORM_BUFFER, 11);
      void BufferParams();
     
};


class ParticleSystem: public Module
{
   public:
      ParticleSystem(int n);

      ComputeShader mComputeShader = ComputeShader("particle_cs.glsl");

      GLuint mShader = -1;
      GLuint mVao;
      std::string mVertexShaderName = std::string("particle_vs.glsl");
      std::string mFragmentShaderName = std::string("particle_fs.glsl");
   
      Buffer mParticleSsbo = Buffer(GL_SHADER_STORAGE_BUFFER, 13);

      struct Particle
      {
         glm::vec4 pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
         glm::vec4 vel = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); //vel.w = particle age
      };

      struct Uniforms
      {
         int mNumParticles = 0;
         float mTime = 0.0f;
      } mUniforms;

      struct UniformLoc
      {
         int mNumParticles = 0;
         int mTime = 1;
      } mUniformLoc;

      void Init() override;
      void Draw() override;
      void DrawGui() override;
      void Animate(float t = -1.0f, float dt = -1.0f) override;
};


class ParticleCollision: public Module
{
   public:
      ParticleCollision(int n);

      ParticleSystem mParticleSystem;
      UniformGridPbdGpu3D* pGrid = nullptr;
      UgridParticles3D mParticleGrid;

      std::string mComputeShaderName = std::string("particle_collision_cs.glsl");
      
      struct Uniforms
      {
         const int PREANIMATE = 3;
         const int COLLISION_QUERY = 4;
         const int POSTANIMATE = 5;

      } mUniforms;

      struct UniformLoc
      {
         int mMode = 3;
      } mUniformLoc;

      struct SsboBinding
      {
         int mGridCounter = 0;
         int mGridOffset = 1;
         int mIndexList = 2;
         int mObj = 3;
         int mParticles = 4;
      } mSsboBinding;


      void Init() override;
      void Draw() override;
      void DrawGui() override;
      void Animate(float t = -1.0f, float dt = -1.0f) override;
};

