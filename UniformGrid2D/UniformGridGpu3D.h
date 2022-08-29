#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>
#include "UniformGrid2D.h"
#include "ParallelScan.h"
#include "Module.h"


class UniformGridPbdGpu3D
{
   public:
 
      std::string mComputeShaderName = std::string("uniform_grid_pbd_3D_cs.glsl");
      GLuint mComputeShader = -1;

      struct UniformGridInfo
      {
         aabb3D mExtents;
         glm::ivec4 mNumCells;
         glm::vec4 mCellSize;
      } mUniformGridInfo;

      GLuint mGridInfoUbo = -1;
      int mGridInfoBinding = 0;

      struct PbdObj
      {
         glm::vec4 color;
         glm::vec4 x;
         glm::vec4 v;
         glm::vec4 p;
         glm::vec4 dp_t;
         glm::vec4 dp_n;
         glm::vec4 hw;
         float rm;
         float n;
         float e;
         float padding;
      };

      //Physics parameters
      struct Params
      {
         glm::vec4 g = glm::vec4(0.0f, -0.1f, 0.0f, 0.0f); //gravity
         float dt = 0.02f;
         float damping = 0.999f;
         float k = 1.75f; //SOR parameter
         float k_static = 10.0f;
         float sleep_thresh = 0.0005f;
         float mu_k = 0.4f; //kinetic friction
         float mu_s = 0.5f; //static friction
         float e = 0.75f; //coefficient of restitution
         int n = 0; //num objects
         int substeps = 1; //number of collision substeps
      } mParams;

      GLuint mParamsUbo = -1;
      int mParamsBinding = 1;

      struct Ssbo
      {
         GLuint mZeros = -1; 
         GLuint mGridCounter = -1; //Number of boxes in each cell's list
         GLuint mGridOffset = -1; //Where in the index list this cell's list starts
         GLuint mIndexList = -1;
         GLuint mObj = -1;
      } mSsbo;

      struct SsboBinding
      {
         int mGridCounter = 0;
         int mGridOffset = 1;
         int mIndexList = 2;
         int mObj = 3;
      } mSsboBinding;

      struct Uniforms
      {
         const int COMPUTE_COUNTS = 0;
         const int COMPUTE_OFFSETS = 1;
         const int INSERT_BOXES = 2;
         const int PREANIMATE = 3;
         const int COLLISION_QUERY = 4;
         const int POSTANIMATE = 5;
         float mTime = 0.0f;
      } mUniforms;

      struct UniformLoc
      {
         int mMode = 0;
         float mTime = 1;
      } mUniformLoc;

      const int mMaxWorkGroup = 1024;
      int mObjWorkgroups = 0;

      ParallelScan mParallelScan;

      UniformGridPbdGpu3D(glm::ivec4 num_cells, aabb3D extents);
      void Init(std::vector<PbdObj>& obj);
      void CollisionQuery();
      void Animate();

      void ClearCounter();
      void ClearOffset();

      void BufferParams();
      
      //Below are for testing
      int Index(int i, int j, int k);
      glm::ivec3 ComputeCellIndex(glm::vec3 p);
      bool collide_ray(glm::vec3 pos, glm::vec3 dir);
};


class GridTestPbdGpu3D: public Module
{
   public:
      GridTestPbdGpu3D(int n);
      UniformGridPbdGpu3D mGrid;

      int mNumObjects;
      
      GLuint mShader = -1;
      GLuint mAttriblessVao = -1;

      struct Uniforms
      {
         int mUseAo = 0;
         float mTime = 0.0f;
      } mUniforms;

      struct UniformLoc
      {
         int mUseAo = 1;
         int mTime = 2;
      } mUniformLoc;

      void Init() override;
      void Draw() override;
      void DrawGui() override;
      void Animate(float t = -1.0f, float dt = -1.0f) override;
};


