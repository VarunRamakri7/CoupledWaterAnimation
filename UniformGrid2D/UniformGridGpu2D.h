#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>
#include "UniformGrid2D.h"
#include "ParallelScan.h"
#include "Module.h"
#include <string>

class UniformGridGpu2D
{
   public:
 
      std::string mComputeShaderName = std::string("uniform_grid_cs.glsl");
      GLuint mComputeShader = -1;

      std::vector<aabb2D> mBoxes;
      
      struct UniformGridInfo
      {
         aabb2D mExtents;
         glm::ivec2 mNumCells;
         glm::vec2 mCellSize;
      } mUniformGridInfo;

      GLuint mGridInfoUbo = -1;
      int mGridInfoBinding = 0;

      struct Ssbo
      {
         GLuint mZeros = -1; 
         GLuint mGridCounter = -1; //Number of boxes in each cell's list
         GLuint mGridOffset = -1; //Where in the index list this cell's list starts
         GLuint mIndexList = -1;
         GLuint mBoxes = -1;
         GLuint mVelocity = -1;
         GLuint mColor = -1;
      } mSsbo;

      struct SsboBinding
      {
         int mGridCounter = 0;
         int mGridOffset = 1;
         int mIndexList = 2;
         int mBoxes = 3;
         int mVelocity = 4;
         int mColor = 5;
      } mSsboBinding;

      struct Uniforms
      {
         const int COMPUTE_COUNTS = 0;
         const int COMPUTE_OFFSETS = 1;
         const int INSERT_BOXES = 2;
         const int COLLISION_QUERY = 3;
         const int ANIMATE = 4;
         //int mMode;
      } mUniforms;

      struct UniformLoc
      {
         int mMode = 0;
         int mNumBoxes = 1;
      } mUniformLoc;

      const int mMaxWorkGroup = 1024;
      int mBoxWorkgroups = 0; 

      ParallelScan mParallelScan;

      UniformGridGpu2D(glm::ivec2 num_cells, aabb2D extents);
      void Init(std::vector<aabb2D>& boxes);
      void CollisionQuery();
      void Animate();

      void ClearCounter();
      void ClearOffset();

      //glm::ivec2 ComputeCellIndex(glm::vec2 pt);
};


class GridTestGpuAnimate: public Module
{
   public:
      GridTestGpuAnimate(int n);
      UniformGridGpu2D mGrid;
      std::vector<aabb2D> mBoxes;
      std::vector<glm::vec2> mVelocity;

      std::vector<int> mAllCollisionIndices;

      GLuint mShader = -1;
      GLuint mAttriblessVao = -1;

      struct Ssbo
      {
         GLuint mBoxes = -1;
         GLuint mColors = -1;
      } mSsbo;

      struct SsboBinding
      {
         int mBoxes = 0;
         int mColors = 1;
      } mSsboBinding;

      struct UniformLocs
      {
         int color = 0;
      } mUniformLocs;

      void Init() override;
      void Draw() override;
      void Animate(float t = -1.0f, float dt = -1.0f) override;
};





class UniformGridPbdGpu2D
{
   public:
 
      std::string mComputeShaderName = std::string("uniform_grid_pbd_cs.glsl");
      GLuint mComputeShader = -1;

      struct UniformGridInfo
      {
         aabb2D mExtents;
         glm::ivec2 mNumCells;
         glm::vec2 mCellSize;
      } mUniformGridInfo;

      GLuint mGridInfoUbo = -1;
      int mGridInfoBinding = 0;

      struct PbdObj
      {
         glm::vec4 color;
         glm::vec2 x;
         glm::vec2 v;
         glm::vec2 p;
         glm::vec2 dp_t;
         glm::vec2 dp_n;
         glm::vec2 hw;
         float rm;
         float n;
         float e;
         float padding;
      };

      //Physics parameters
      struct Params
      {
         glm::vec2 g = glm::vec2(0.0f, -0.1f); //gravity
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
         
      } mUniforms;

      struct UniformLoc
      {
         int mMode = 0;
      } mUniformLoc;

      const int mMaxWorkGroup = 1024;
      int mObjWorkgroups = 0; 

      ParallelScan mParallelScan;

      UniformGridPbdGpu2D(glm::ivec2 num_cells, aabb2D extents);
      void Init(std::vector<PbdObj>& obj);
      void CollisionQuery();
      void Animate();

      void ClearCounter();
      void ClearOffset();

};


class GridTestPbdGpu2D: public Module
{
   public:
      GridTestPbdGpu2D(int n);
      UniformGridPbdGpu2D mGrid;

      int mNumObjects;
      
      GLuint mShader = -1;
      GLuint mAttriblessVao = -1;

      void Init() override;
      void Draw() override;
      void Animate(float t = -1.0f, float dt = -1.0f) override;
};