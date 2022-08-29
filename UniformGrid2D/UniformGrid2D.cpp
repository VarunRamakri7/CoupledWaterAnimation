#include "UniformGrid2D.h"
#include "InitShader.h"
#include "imgui.h"
#include <GLFW/glfw3.h>



UniformGrid2D::UniformGrid2D(glm::ivec2 num_cells, aabb2D extents):
   mNumCells(num_cells),
   mExtents(extents),
   mGridCounter(num_cells.x*num_cells.y, 0),
   mGridOffset(num_cells.x*num_cells.y, 0)
{
   mCellSize = (mExtents.mMax-mExtents.mMin)/glm::vec2(mNumCells);
}

void UniformGrid2D::ClearCounter()
{
   for(int i=0; i<mNumCells.x*mNumCells.y; i++)
   {
      mGridCounter[i] = 0;
   }
}

glm::ivec2 UniformGrid2D::ComputeCellIndex(glm::vec2 pt)
{
   pt = pt-mExtents.mMin;
   glm::ivec2 cell = glm::floor(pt/mCellSize);
   cell = glm::clamp(cell, glm::ivec2(0), mNumCells-glm::ivec2(1));
   return cell;
}

void UniformGrid2D::Build(std::vector<aabb2D>& boxes)
{
   mBoxes = boxes;

   //clear counter and offsets
   ClearCounter();

   for(int i=0; i<mNumCells.x*mNumCells.y; i++)
   {
      mGridOffset[i] = 0; 
   }

   //Compute Counts
   for(int b=0; b<boxes.size(); b++)
   {
      glm::ivec2 cell_min = ComputeCellIndex(boxes[b].mMin);
      glm::ivec2 cell_max = ComputeCellIndex(boxes[b].mMax);

      for(int i=cell_min.x; i<=cell_max.x; i++)
      {
         for(int j=cell_min.y; j<=cell_max.y; j++)
         {
            RefGridCounter(i,j)++;
         }
      }
   }

   //Compute Offsets (exclusive prefix sum of counts)
   int sum = 0;
   for(int i=0; i<mNumCells.x; i++)
   {
      for(int j=0; j<mNumCells.y; j++)
      {
         RefGridOffset(i,j) = sum;
         sum += RefGridCounter(i,j); 
      }
   }

   mIndexList.resize(sum);
   ClearCounter();

   //Insert boxes into mBoxList
   for(int b=0; b<boxes.size(); b++)
   {
      glm::ivec2 cell_min = ComputeCellIndex(boxes[b].mMin);
      glm::ivec2 cell_max = ComputeCellIndex(boxes[b].mMax);

      //insert box into each cell it overlaps
      for(int i=cell_min.x; i<=cell_max.x; i++)
      {
         for(int j=cell_min.y; j<=cell_max.y; j++)
         {
            int offset = RefGridOffset(i,j);
            int count = RefGridCounter(i,j);
            mIndexList[offset+count] = b;
            RefGridCounter(i,j)++;
         }
      }
   }
}

std::vector<int> UniformGrid2D::Query(glm::vec2 p)
{
   std::vector<int> result;

   //This is the cell the query overlaps
   glm::ivec2 cell = ComputeCellIndex(p);

   //Insert the list of the overlapped cell
   int offset = RefGridOffset(cell.x, cell.y);
   int count = RefGridCounter(cell.x, cell.y);
   for(int list_index = offset; list_index<offset+count; list_index++)
   {
      int box_index = mIndexList[list_index];
      if(point_in(p, mBoxes[box_index]))
      {
         result.push_back(box_index);
      }
   }       

   return result;  
}

std::vector<int> UniformGrid2D::QueryWithDuplicates(aabb2D query_aabb)
{
   std::vector<int> result;
   //These are the cells the query overlaps
   glm::ivec2 cell_min = ComputeCellIndex(query_aabb.mMin);
   glm::ivec2 cell_max = ComputeCellIndex(query_aabb.mMax);

   for(int i=cell_min.x; i<=cell_max.x; i++)
   {
      for(int j=cell_min.y; j<=cell_max.y; j++)
      {
         //Insert the list for each overlapped cell
         int offset = RefGridOffset(i, j);
         int count = RefGridCounter(i, j);
         for(int list_index = offset; list_index<offset+count; list_index++)
         {
            int box_index = mIndexList[list_index];
            if(overlap(query_aabb, mBoxes[box_index]))
            {
               result.push_back(box_index);
            }
         }       
      }
   }

   return result;  
}

std::vector<int> UniformGrid2D::Query(aabb2D query_aabb)
{
   std::vector<int> result;
   //These are the cells the query overlaps
   glm::ivec2 cell_min = ComputeCellIndex(query_aabb.mMin);
   glm::ivec2 cell_max = ComputeCellIndex(query_aabb.mMax);

   for(int i=cell_min.x; i<=cell_max.x; i++)
   {
      for(int j=cell_min.y; j<=cell_max.y; j++)
      {
         //Insert the list for each overlapped cell
         int offset = RefGridOffset(i, j);
         int count = RefGridCounter(i, j);
         for(int list_index = offset; list_index<offset+count; list_index++)
         {
            int box_index = mIndexList[list_index];
            aabb2D box = mBoxes[box_index];
            glm::ivec2 home = glm::max(cell_min, ComputeCellIndex(box.mMin));
            if(home == glm::ivec2(i,j) && overlap(query_aabb, box))
            {
               result.push_back(box_index);
            }
         }       
      }
   }

   return result;  
}

#include <random>
GridTestSelect::GridTestSelect(int n):mGrid(glm::ivec2(20, 20), aabb2D(glm::vec3(-1.0f), glm::vec3(+1.0f)))
{
   mBoxes.resize(n);
   std::default_random_engine gen;
   std::uniform_real_distribution<float> urand_pos(-1.0f, 1.0f);
   std::uniform_real_distribution<float> urand_size(0.01f, 0.05f);

   for(int i=0; i<mBoxes.size(); i++)
   {
      glm::vec2 cen = glm::vec2(urand_pos(gen), urand_pos(gen));
      glm::vec2 h = glm::vec2(urand_size(gen), urand_size(gen));
      mBoxes[i] = aabb2D(cen-h, cen+h);
   }

   mGrid.Build(mBoxes);
}

void GridTestSelect::Init()
{
   std::string vertex_shader = "grid_test_vs.glsl";
   std::string fragment_shader = "grid_test_fs.glsl";
   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   std::vector<int> all_indices(mBoxes.size());
   for(int i=0; i<mBoxes.size(); i++)
   {
      all_indices[i] = i;
   }

   glGenVertexArrays(1, &mAttriblessVao);

   GLuint flags = GL_DYNAMIC_STORAGE_BIT;
   glGenBuffers(1, &mBoxSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBoxSsbo);
   glNamedBufferStorage(mBoxSsbo, sizeof(aabb2D) * mBoxes.size(), mBoxes.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mBoxSsboBinding, mBoxSsbo);

   glGenBuffers(1, &mAllIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mAllIndicesSsbo);
   glNamedBufferStorage(mAllIndicesSsbo, sizeof(int) * all_indices.size(), all_indices.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllIndicesSsbo);

   glGenBuffers(1, &mMouseoverIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mMouseoverIndicesSsbo);
   glNamedBufferStorage(mMouseoverIndicesSsbo, sizeof(int) * all_indices.size(), all_indices.data(), flags);

   glGenBuffers(1, &mSelectedIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSelectedIndicesSsbo);
   glNamedBufferStorage(mSelectedIndicesSsbo, sizeof(int) * all_indices.size(), all_indices.data(), flags);

   glGenBuffers(1, &mSelectedCollisionIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSelectedCollisionIndicesSsbo);
   glNamedBufferStorage(mSelectedCollisionIndicesSsbo, sizeof(int) * mBoxes.size() * 4, nullptr, flags);

}

void GridTestSelect::Draw()
{
   glm::vec4 default_color(0.0f, 1.0f, 0.0f, 1.0f);
   glm::vec4 mouseover_color(1.0f, 0.0f, 0.0f, 1.0f);
   glm::vec4 selected_color(1.0f, 1.0f, 0.0f, 1.0f);

   glUseProgram(mShader);
   glBindVertexArray(mAttriblessVao);

   glDisable(GL_DEPTH_TEST);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllIndicesSsbo);
   glProgramUniform4fv(mShader, mUniformLocs.color, 1, &default_color.r);
   glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mBoxes.size());

   if(mMouseoverBoxIndices.size() > 0)
   {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mMouseoverIndicesSsbo);
      glProgramUniform4fv(mShader, mUniformLocs.color, 1, &mouseover_color.r);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mMouseoverBoxIndices.size());
   }

   if(mSelectedBoxIndices.size() > 0)
   {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mSelectedIndicesSsbo);
      glProgramUniform4fv(mShader, mUniformLocs.color, 1, &selected_color.r);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mSelectedBoxIndices.size());
   }

   if(mSelectedCollisionIndices.size() > 0)
   {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mSelectedCollisionIndicesSsbo);
      glProgramUniform4fv(mShader, mUniformLocs.color, 1, &selected_color.r);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mSelectedCollisionIndices.size());
   }

   glEnable(GL_DEPTH_TEST);
}

void GridTestSelect::MouseCursor(glm::vec2 pos)
{
   static glm::vec2 last_pos(-10.0f);
   glm::vec2 delta = pos-last_pos;

   //aabb2D mouse(pos-glm::vec2(0.1f), pos+glm::vec2(0.1f));
   //mMouseoverBoxList = mGrid.QueryWithDuplicates(mouse);

   mMouseoverBoxIndices = mGrid.Query(pos);
   int offset = 0;
   glNamedBufferSubData(mMouseoverIndicesSsbo, offset, sizeof(int) * mMouseoverBoxIndices.size(), mMouseoverBoxIndices.data());

   if(last_pos != glm::vec2(-10.0f))
   if(mMouseoverBoxIndices.size() > 0)
   {
      //Move the dragged boxes
      for(int i=0; i<mSelectedBoxIndices.size(); i++)
      {
         int ix = mSelectedBoxIndices[i];
         mBoxes[ix].mMin += delta;
         mBoxes[ix].mMax += delta;  
      }
      int offset = 0;
      glNamedBufferSubData(mBoxSsbo, offset, sizeof(aabb2D) * mBoxes.size(), mBoxes.data());
      mGrid.Build(mBoxes);


      //update selection collisions
      mSelectedCollisionIndices.resize(0);
      for(int i=0; i<mSelectedBoxIndices.size(); i++)
      {
         int ix = mSelectedBoxIndices[i];
         aabb2D box_ix = mBoxes[ix];
         std::vector<int> collide_i = mGrid.Query(box_ix);
         mSelectedCollisionIndices.insert(mSelectedCollisionIndices.end(), collide_i.begin(), collide_i.end());
      }
      if(mSelectedCollisionIndices.size() > 0)
      {
         glNamedBufferSubData(mSelectedCollisionIndicesSsbo, offset, sizeof(int) * mSelectedCollisionIndices.size(), mSelectedCollisionIndices.data());
      }
   }
   last_pos = pos;
}


void GridTestSelect::MouseButton(int button, int action, int mods, glm::vec2 pos)
{
   if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
   {
      mSelectedBoxIndices = mGrid.Query(pos);
      int offset = 0;
      glNamedBufferSubData(mSelectedIndicesSsbo, offset, sizeof(int) * mSelectedBoxIndices.size(), mSelectedBoxIndices.data());
   }
   if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
   {
      mSelectedBoxIndices.resize(0);
   }
}



















/////////////////////////////////////////////////////////////////////////////////////////////////////////














GridTestAnimate::GridTestAnimate(int n):mGrid(glm::ivec2(20, 20), aabb2D(glm::vec3(-1.0f), glm::vec3(+1.0f)))
{
   mBoxes.resize(n);
   std::default_random_engine gen;
   std::uniform_real_distribution<float> urand_pos(-1.0f, 1.0f);
   std::uniform_real_distribution<float> urand_size(0.002f, 0.01f);

   for(int i=0; i<mBoxes.size(); i++)
   {
      glm::vec2 cen = glm::vec2(0.9f*urand_pos(gen), 0.9f*urand_pos(gen));
      glm::vec2 h = glm::vec2(urand_size(gen), urand_size(gen));
      mBoxes[i] = aabb2D(cen-h, cen+h);
   }

   mVelocity.resize(n);
   for(int i=0; i<mBoxes.size(); i++)
   {
      glm::vec2 vel = glm::vec2(urand_pos(gen), urand_pos(gen));
      mVelocity[i] = 0.1f*vel;
   }

   //mBoxes[n-1].mMin = glm::vec2(-0.05f);
   //mBoxes[n-1].mMax = glm::vec2(+0.05f);
   //mVelocity[n-1] = glm::vec2(0.0f);

   mGrid.Build(mBoxes);
}

void GridTestAnimate::Init()
{
   std::string vertex_shader = "grid_test_vs.glsl";
   std::string fragment_shader = "grid_test_fs.glsl";
   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   std::vector<int> all_indices(mBoxes.size());
   for(int i=0; i<mBoxes.size(); i++)
   {
      all_indices[i] = i;
   }

   glGenVertexArrays(1, &mAttriblessVao);

   GLuint flags = GL_DYNAMIC_STORAGE_BIT;
   glGenBuffers(1, &mBoxSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBoxSsbo);
   glNamedBufferStorage(mBoxSsbo, sizeof(aabb2D) * mBoxes.size(), mBoxes.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mBoxSsboBinding, mBoxSsbo);

   glGenBuffers(1, &mAllIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mAllIndicesSsbo);
   glNamedBufferStorage(mAllIndicesSsbo, sizeof(int) * all_indices.size(), all_indices.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllIndicesSsbo);

   glGenBuffers(1, &mAllCollisionIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mAllCollisionIndicesSsbo);
   glNamedBufferStorage(mAllCollisionIndicesSsbo, sizeof(int) * mBoxes.size() * 4, nullptr, flags);
}

void GridTestAnimate::Draw()
{
   glm::vec4 default_color(0.0f, 1.0f, 0.0f, 1.0f);
   glm::vec4 mouseover_color(1.0f, 0.0f, 0.0f, 1.0f);
   glm::vec4 selected_color(1.0f, 1.0f, 0.0f, 1.0f);

   glUseProgram(mShader);
   glBindVertexArray(mAttriblessVao);

   glDisable(GL_DEPTH_TEST);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllIndicesSsbo);
   glProgramUniform4fv(mShader, mUniformLocs.color, 1, &default_color.r);
   glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mBoxes.size());

   UpdateAllCollisions();
   if(mAllCollisionIndices.size() > 0)
   {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllCollisionIndicesSsbo);
      glProgramUniform4fv(mShader, mUniformLocs.color, 1, &selected_color.r);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mAllCollisionIndices.size());
   }

   Animate();

   glEnable(GL_DEPTH_TEST);
}

void GridTestAnimate::UpdateAllCollisions()
{
   //update selection collisions
   mAllCollisionIndices.resize(0);
   for(int i=0; i<mBoxes.size(); i++)
   {
      aabb2D box_i = mBoxes[i];
      std::vector<int> collide_i = mGrid.Query(box_i);
      
      for(int j=0; j<collide_i.size(); j++)
      {
         if(collide_i[j] < i)
         {
            mAllCollisionIndices.push_back(i);
            mAllCollisionIndices.push_back(collide_i[j]);
            Collide(i, collide_i[j]);
         }
      }
   }
   if(mAllCollisionIndices.size() > 0)
   {
      int offset = 0;
      glNamedBufferSubData(mAllCollisionIndicesSsbo, offset, sizeof(int) * mAllCollisionIndices.size(), mAllCollisionIndices.data());   
   }
}

void GridTestAnimate::Collide(int i, int j)
{
   aabb2D isect = intersection(mBoxes[i], mBoxes[j]); //overlap rectangle
   glm::vec2 isect_size = isect.mMax-isect.mMin;
   int comp = isect_size.x < isect_size.y ? 0:1;
   float dist = 0.5f*isect_size[comp];

   glm::vec2 cen_i = 0.5f*(mBoxes[i].mMin + mBoxes[i].mMax);
   glm::vec2 cen_j = 0.5f*(mBoxes[j].mMin + mBoxes[j].mMax);

   //glm::vec2 dij = mBoxes[j].mMin - mBoxes[i].mMin; //points from i to j
   glm::vec2 dij = cen_j - cen_i; //points from i to j
   glm::vec2 ud = normalize(dij);
   glm::vec2 disp_j(0.0f);
   disp_j[comp] = glm::sign(dij[comp])*dist;

   mBoxes[j].mMin += disp_j;
   mBoxes[j].mMax += disp_j;
   

   mBoxes[i].mMin -= disp_j;
   mBoxes[i].mMax -= disp_j;

   //mVelocity[j][comp] *= -0.9f; 
   //mVelocity[i][comp] *= -0.9f; 

   float mi = area(mBoxes[i]);
   float mj = area(mBoxes[j]);
   
   float f = -(1.0f+mParams.e)*glm::dot(ud, mVelocity[i]-mVelocity[j])/(1.0f/mi+1.0f/mj);
   
   mVelocity[j] -= f*ud; 
   mVelocity[i] += f*ud; 
}

void GridTestAnimate::Animate(float t, float dt)
{
   for(int i=0; i<mBoxes.size(); i++)
   {
      glm::vec2 d = mVelocity[i]*mParams.dt;
      mBoxes[i].mMin += d;
      mBoxes[i].mMax += d;

      //gravity
      mVelocity[i].y -= 0.05*mParams.dt;

      //Collide with extents
      for(int c = 0; c<2; c++)
      {
         float overlap = mGrid.mExtents.mMin[c]-mBoxes[i].mMin[c];
         if(overlap > 0.0f)
         {
            mBoxes[i].mMin[c] += overlap;
            mBoxes[i].mMax[c] += overlap;
            mVelocity[i][c] *= -mParams.e;
         }

         overlap = mBoxes[i].mMax[c] - mGrid.mExtents.mMax[c];
         if(overlap > 0.0f)
         {
            mBoxes[i].mMin[c] -= overlap;
            mBoxes[i].mMax[c] -= overlap;
            mVelocity[i][c] *= -mParams.e;
         }
      }
   }
   mGrid.Build(mBoxes);
   int offset = 0;
   glNamedBufferSubData(mBoxSsbo, offset, sizeof(aabb2D) * mBoxes.size(), mBoxes.data());
}

























GridTestPbd::GridTestPbd(int n):mGrid(glm::ivec2(20, 20), aabb2D(glm::vec3(-1.0f), glm::vec3(+1.0f)))
{
   mParams.n = n;
}

void GridTestPbd::Init()
{
   std::default_random_engine gen;
   std::uniform_real_distribution<float> urand_pos(-1.0f, 1.0f);
   std::uniform_real_distribution<float> urand_size(0.002f, 0.01f);

   const int n = mParams.n;
      mObjects.resize(n);
      for(int i=0; i<mObjects.size(); i++)
      {
         glm::vec2 x = glm::vec2(0.5f*urand_pos(gen), 0.5f*urand_pos(gen));
         glm::vec2 v = glm::vec2(urand_pos(gen), urand_pos(gen));
         glm::vec2 h = glm::vec2(urand_size(gen), urand_size(gen));
      

         mObjects[i].x = x;
         mObjects[i].v = 0.1f*v;
         mObjects[i].p = x;
         mObjects[i].dp_t = glm::vec2(0.0f);
         mObjects[i].dp_n = glm::vec2(0.0f);
         mObjects[i].hw = h;
         mObjects[i].rm = 1.0f/(h.x*h.y);
         mObjects[i].n = 0.0f;
         mObjects[i].e = 1.0f;
      }

      mObjects[n-1].hw = glm::vec2(0.06f);
      mObjects[n-1].rm = 1.0f/(mObjects[n-1].hw.x*mObjects[n-1].hw.y);

   mBoxes.resize(n);
   for(int i=0; i<mBoxes.size(); i++)
   {
      mBoxes[i] = aabb2D(mObjects[i].x-mObjects[i].hw, mObjects[i].x-mObjects[i].hw);
   }

   mGrid.Build(mBoxes);


   std::string vertex_shader = "grid_test_vs.glsl";
   std::string fragment_shader = "grid_test_fs.glsl";

   if(mShader != -1)
   {
      glDeleteProgram(mShader);   
   }
   mShader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   std::vector<int> all_indices(mBoxes.size());
   for(int i=0; i<mBoxes.size(); i++)
   {
      all_indices[i] = i;
   }

   if(mAttriblessVao==-1)
   {
      glGenVertexArrays(1, &mAttriblessVao);
   }

   if(mBoxSsbo != -1)
   {
      glDeleteBuffers(1, &mBoxSsbo);   
   }

   GLuint flags = GL_DYNAMIC_STORAGE_BIT;
   glGenBuffers(1, &mBoxSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBoxSsbo);
   glNamedBufferStorage(mBoxSsbo, sizeof(aabb2D) * mBoxes.size(), mBoxes.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mBoxSsboBinding, mBoxSsbo);

   if(mAllIndicesSsbo != -1)
   {
      glDeleteBuffers(1, &mAllIndicesSsbo);   
   }

   glGenBuffers(1, &mAllIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mAllIndicesSsbo);
   glNamedBufferStorage(mAllIndicesSsbo, sizeof(int) * all_indices.size(), all_indices.data(), flags);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllIndicesSsbo);

   if(mAllCollisionIndicesSsbo != -1)
   {
      glDeleteBuffers(1, &mAllCollisionIndicesSsbo);   
   }

   glGenBuffers(1, &mAllCollisionIndicesSsbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, mAllCollisionIndicesSsbo);
   glNamedBufferStorage(mAllCollisionIndicesSsbo, sizeof(int) * mBoxes.size() * 4, nullptr, flags);
}

void GridTestPbd::Draw()
{
   glm::vec4 default_color(0.0f, 1.0f, 0.0f, 1.0f);
   glm::vec4 mouseover_color(1.0f, 0.0f, 0.0f, 1.0f);
   glm::vec4 selected_color(1.0f, 1.0f, 0.0f, 1.0f);

   glUseProgram(mShader);
   glBindVertexArray(mAttriblessVao);

   glDisable(GL_DEPTH_TEST);

   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllIndicesSsbo);
   glProgramUniform4fv(mShader, mUniformLocs.color, 1, &default_color.r);
   glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mBoxes.size());

//*
   if(mAllCollisionIndices.size() > 0)
   {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mIndexSsboBinding, mAllCollisionIndicesSsbo);
      glProgramUniform4fv(mShader, mUniformLocs.color, 1, &selected_color.r);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mAllCollisionIndices.size());
   }
//*/
   Animate();

   glEnable(GL_DEPTH_TEST);
}

void GridTestPbd::Animate(float t, float dt)
{
   //Animate 1
   for(int i=0; i<mObjects.size(); i++)
   {
      //Apply external forces, damping
      mObjects[i].v = mParams.damping*(mObjects[i].v + mParams.dt*mParams.g);

      //Zero out the accumulators
      mObjects[i].dp_n = glm::vec2(0.0f); 
      mObjects[i].dp_t = glm::vec2(0.0f); //displacements from constraints
      mObjects[i].n = 0.0f; //number of constraints applied

      //Compute estimates
      mObjects[i].p = mObjects[i].x + mParams.dt*mObjects[i].v;
   }

   for(int i=0; i<mParams.substeps; i++) //substeps not working
   {
      ProjectConstraints();
   }

   //Animate 2
   for(int i=0; i<mObjects.size(); i++)
   {
      glm::vec2 pe = mObjects[i].p;
      if(mObjects[i].n > 0.0f)
      {
         mObjects[i].p += mParams.k*(mObjects[i].dp_t + mObjects[i].dp_n)/mObjects[i].n;
         pe += mParams.k*(mObjects[i].dp_t + mParams.e*mObjects[i].dp_n)/mObjects[i].n;
      }
      //mObjects[i].v = (mObjects[i].p - mObjects[i].x)/dt;
      mObjects[i].v = (pe - mObjects[i].x)/mParams.dt;

      //sleeping
      if(glm::length(mObjects[i].p - mObjects[i].x) > mParams.sleep_thresh)
      {
         mObjects[i].x = mObjects[i].p;
      }
   }
}


void GridTestPbd::ProjectConstraints()
{
   for(int i=0; i<mBoxes.size(); i++)
   {
      mBoxes[i].mMin = mObjects[i].p - mObjects[i].hw;
      mBoxes[i].mMax = mObjects[i].p + mObjects[i].hw;
   }
   mGrid.Build(mBoxes);
   
   int offset = 0;
   glNamedBufferSubData(mBoxSsbo, offset, sizeof(aabb2D) * mBoxes.size(), mBoxes.data());

   //update collisions
   mAllCollisionIndices.resize(0);

   for(int i=0; i<mBoxes.size(); i++)
   {
      aabb2D box_i = mBoxes[i];

      //Collide with extents
      //const float k_static = 10.0f;
      for(int c = 0; c<2; c++) //dimension: x or y
      {
/*
         float overlap = mGrid.mExtents.mMin[c]-mBoxes[i].mMin[c];
         if(overlap > 0.0f)
         {
            mObjects[i].dp_n[c] += k_static*overlap;
            mObjects[i].n += k_static;

            glm::vec2 dp_perp = (mObjects[i].x-mObjects[i].p);
            dp_perp[c] = 0.0;

            glm::vec2 frict_j = dp_perp;
            float dp_perp_mag = glm::length(dp_perp);
            if(dp_perp_mag < mParams.mu_s*overlap)
            {
               frict_j = frict_j*glm::min(1.0f, mParams.mu_k*overlap/dp_perp_mag);
            }
            mObjects[i].dp_t += frict_j;
         }

         overlap = mBoxes[i].mMax[c] - mGrid.mExtents.mMax[c];
         if(overlap > 0.0f)
         {
            mObjects[i].dp_n[c] -= k_static*overlap;
            mObjects[i].n += k_static;

            glm::vec2 dp_perp = (mObjects[i].x-mObjects[i].p);
            dp_perp[c] = 0.0;
            glm::vec2 frict_j = dp_perp;
            float dp_perp_mag = glm::length(dp_perp);
            if(dp_perp_mag < mParams.mu_s*overlap)
            {
               frict_j = frict_j*glm::min(1.0f, mParams.mu_k*overlap/dp_perp_mag);
            }
            mObjects[i].dp_t += frict_j;
         }
*/
         float overlap[2] = {mGrid.mExtents.mMin[c]-mBoxes[i].mMin[c], mBoxes[i].mMax[c] - mGrid.mExtents.mMax[c]};
         float dir[2] = {+1.0f, -1.0f};
         for(int m=0; m<2; m++)
         {
            if(overlap[m] > 0.0f)
            {
               mObjects[i].dp_n[c] += dir[m]*mParams.k_static*overlap[m];
               mObjects[i].n += mParams.k_static;

               glm::vec2 dp_perp = (mObjects[i].x-mObjects[i].p);
               dp_perp[c] = 0.0;
               glm::vec2 frict_j = dp_perp;
               float dp_perp_mag = glm::length(dp_perp);
               if(dp_perp_mag < mParams.mu_s*overlap[m])
               {
                  frict_j = frict_j*glm::min(1.0f, mParams.mu_k*overlap[m]/dp_perp_mag);
               }
               mObjects[i].dp_t += frict_j;
            }
         }
      }

      std::vector<int> collide_i = mGrid.Query(box_i);
      for(int j=0; j<collide_i.size(); j++)
      {
         if(collide_i[j] < i)
         {
            mAllCollisionIndices.push_back(i);
            mAllCollisionIndices.push_back(collide_i[j]);
            Collide(i, collide_i[j]);
         }
      }
   }
//*
   if(mAllCollisionIndices.size() > 0)
   {
      int offset = 0;
      glNamedBufferSubData(mAllCollisionIndicesSsbo, offset, sizeof(int) * mAllCollisionIndices.size(), mAllCollisionIndices.data());   
   }
//*/
}

void GridTestPbd::Collide(int i, int j)
{
   aabb2D isect = intersection(mBoxes[i], mBoxes[j]); //overlap rectangle
   glm::vec2 isect_size = isect.mMax-isect.mMin;
   int comp = isect_size.x < isect_size.y ? 0:1; //direction to separate
   float dist = 0.5f*isect_size[comp];
   if(dist < 0.0f) return;

   glm::vec2 cen_i = mObjects[i].p;
   glm::vec2 cen_j = mObjects[j].p;

   float eps = 1e-8f;
   glm::vec2 dij = cen_j - cen_i; //vector points from i to j
   glm::vec2 ud = dij/(glm::length(dij)+eps);
   glm::vec2 disp_j(0.0f);
   disp_j[comp] = glm::sign(dij[comp])*dist;

   const float rmi = mObjects[i].rm;
   const float rmj = mObjects[j].rm;
   const float wi = rmi/(rmi+rmj);
   const float wj = rmj/(rmi+rmj);

   mObjects[j].dp_n += wj*disp_j;
   mObjects[j].n += 1.0f;

   mObjects[i].dp_n -= wi*disp_j;
   mObjects[i].n += 1.0f;

   //friction
   glm::vec2 dp_perp = ((mObjects[j].x-cen_j)-(mObjects[i].x-cen_i));
   dp_perp = dp_perp - glm::dot(dp_perp, ud)*ud;

   glm::vec2 frict_j = dp_perp;
   float dp_perp_mag = glm::length(dp_perp);
   if(dp_perp_mag < mParams.mu_s*dist)
   {
      frict_j = frict_j*glm::min(1.0f, mParams.mu_k*dist/dp_perp_mag);
   }

   mObjects[j].dp_t += wj*frict_j;
   mObjects[i].dp_t -= wi*frict_j;
}

void GridTestPbd::MouseButton(int button, int action, int mods, glm::vec2 p)
{
   if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
   {
      std::vector<int> collide = mGrid.Query(p);
      if(collide.size() > 0)
      {
         mSelectedBox = collide[0];
      }
      else
      {
         mSelectedBox = -1;
      }
   }

   if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
   {
      mSelectedBox = -1; //deselect
   }
}

void GridTestPbd::MouseCursor(glm::vec2 pos)
{
   static glm::vec2 last_pos(-10.0f);
   glm::vec2 delta = pos-last_pos;

   if(last_pos != glm::vec2(-10.0f) && mSelectedBox >= 0)
   {
      mObjects[mSelectedBox].v = pos-mObjects[mSelectedBox].x;
      mObjects[mSelectedBox].x = pos;
   }
   last_pos = pos;
}


void GridTestPbd::DrawGui()
{
   ImGui::Begin("GridTestPdb");
      if(ImGui::Button("Reinit"))
      {
         Init();
      }

      ImGui::SliderFloat2("g", &mParams.g.x, -0.2f, +0.2f);
      ImGui::SliderFloat("dt", &mParams.dt, 0.0f, +0.1f);
      ImGui::SliderFloat("damping", &mParams.damping, 0.9f, 1.0f, "%0.4f");
      ImGui::SliderFloat("k (SOR)", &mParams.k, 1.0f, 2.0f);
      ImGui::SliderFloat("k static", &mParams.k_static, 1.0f, 10.0f);
      ImGui::SliderFloat("sleep thresh", &mParams.sleep_thresh, 0.0f, +0.001f, "%0.4f");
      ImGui::SliderFloat("mu k", &mParams.mu_k, 0.0f, 1.0f);
      ImGui::SliderFloat("mu s", &mParams.mu_s, 0.0f, 1.0f);
      ImGui::SliderFloat("e", &mParams.e, 0.0f, 1.0f);
      ImGui::SliderInt("substeps", &mParams.substeps, 0, 20);
   ImGui::End();
}




