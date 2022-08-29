#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>
#include "Module.h"
#include "Aabb.h"

class UniformGrid2D
{
   public:
      std::vector<int> mGridCounter; //Number of boxes in each cell's list
      std::vector<int> mGridOffset; //Where in the index list this cell's list starts
      inline int& RefGridCounter(int i, int j) {return mGridCounter[i*mNumCells.y + j];}
      inline int& RefGridOffset(int i, int j) {return mGridOffset[i*mNumCells.y + j];}

      std::vector<aabb2D> mBoxes;
      std::vector<int> mIndexList;
      aabb2D mExtents;
      glm::ivec2 mNumCells;
      glm::vec2 mCellSize;

      UniformGrid2D(glm::ivec2 num_cells, aabb2D extents);
      void Build(std::vector<aabb2D>& boxes);
      std::vector<int> QueryWithDuplicates(aabb2D query_aabb);
      std::vector<int> Query(aabb2D query_aabb);
      std::vector<int> Query(glm::vec2 p);

      void ClearCounter();

      glm::ivec2 ComputeCellIndex(glm::vec2 pt);
};




class GridTestSelect: public Module
{
   public:
      GridTestSelect(int n);
      UniformGrid2D mGrid;
      std::vector<aabb2D> mBoxes;

      std::vector<int> mMouseoverBoxIndices;
      std::vector<int> mSelectedBoxIndices;
      std::vector<int> mSelectedCollisionIndices;

      GLuint mShader = -1;
      GLuint mAttriblessVao = -1;

      GLuint mBoxSsbo = -1;
      GLuint mAllIndicesSsbo = -1;
      GLuint mMouseoverIndicesSsbo = -1;
      GLuint mSelectedIndicesSsbo = -1;
      GLuint mSelectedCollisionIndicesSsbo = -1;

      int mBoxSsboBinding = 0;
      int mIndexSsboBinding = 1;

      struct UniformLocs
      {
         int color = 0;
      } mUniformLocs;

      void Init() override;
      void Draw() override;
      void MouseCursor(glm::vec2 p) override;
      void MouseButton(int button, int action, int mods, glm::vec2 p) override;
};

class GridTestAnimate: public Module
{
   public:
      GridTestAnimate(int n);
      UniformGrid2D mGrid;
      std::vector<aabb2D> mBoxes;
      std::vector<glm::vec2> mVelocity;

      std::vector<int> mAllCollisionIndices;

      struct Params
      {
         float dt = 0.01f;
         float e = 0.9f;
      } mParams;

      GLuint mShader = -1;
      GLuint mAttriblessVao = -1;

      GLuint mBoxSsbo = -1;
      GLuint mAllIndicesSsbo = -1;
      GLuint mAllCollisionIndicesSsbo = -1;

      int mBoxSsboBinding = 0;
      int mIndexSsboBinding = 1  ;

      struct UniformLocs
      {
         int color = 0;
      } mUniformLocs;

      void Init() override;
      void Draw() override;
      void Animate(float t = -1.0f, float dt = -1.0f) override;

      void UpdateAllCollisions();
      void Collide(int i, int j);
};


class GridTestPbd: public Module
{
   public:
      GridTestPbd(int n);
      UniformGrid2D mGrid;
      
      struct PbdObj
      {
         glm::vec2 x;
         glm::vec2 v;
         glm::vec2 p;
         glm::vec2 dp_t;
         glm::vec2 dp_n;
         glm::vec2 hw;
         float rm;
         float n;
         float e;
      };

      std::vector<PbdObj> mObjects;

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
         int substeps = 1; //number of collision substeps (NOT WORKING)
      } mParams;
      
      std::vector<aabb2D> mBoxes;

      std::vector<int> mAllCollisionIndices;

      GLuint mShader = -1;
      GLuint mAttriblessVao = -1;

      GLuint mBoxSsbo = -1;
      GLuint mAllIndicesSsbo = -1;
      GLuint mAllCollisionIndicesSsbo = -1;

      int mBoxSsboBinding = 0;
      int mIndexSsboBinding = 1;

      int mSelectedBox = -1;

      struct UniformLocs
      {
         int color = 0;
      } mUniformLocs;

      void Init() override;
      void Draw() override;
      void DrawGui() override;
      void Animate(float t = -1.0f, float dt = -1.0f) override;
      void MouseCursor(glm::vec2 p) override;
      void MouseButton(int button, int action, int mods, glm::vec2 p) override;
      

      void ProjectConstraints();
      void UpdateVelocities();
      void Collide(int i, int j);

};

