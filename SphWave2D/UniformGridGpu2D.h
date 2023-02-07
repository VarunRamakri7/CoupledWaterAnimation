#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>
#include "Aabb.h"
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

};

class UniformGridSph2D
{
public:

   std::string mComputeShaderName = std::string("uniform_grid_sph_cs.glsl");
   GLuint mComputeShader = -1;

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
      GLuint mParticles = -1;
   } mSsbo;

   struct SsboBinding
   {
      int mParticles = 0;
      int mGridCounter = 2;
      int mGridOffset = 3;
      int mIndexList = 4;
      
   } mSsboBinding;

   struct Uniforms
   {
      const int COMPUTE_COUNTS = 0;
      const int COMPUTE_OFFSETS = 1;
      const int INSERT_BOXES = 2;
   } mUniforms;

   struct UniformLoc
   {
      int mMode = 0;
      int mNumBoxes = 1;
   } mUniformLoc;

   const int mMaxWorkGroup = 1024;
   int mBoxWorkgroups = 0;

   ParallelScan mParallelScan;

   UniformGridSph2D(glm::ivec2 num_cells, aabb2D extents);
   void Init(GLuint particles_ssbo, GLuint particles_binding, int num_particles);
   void CollisionQuery();

   void ClearCounter();
   void ClearOffset();

};
