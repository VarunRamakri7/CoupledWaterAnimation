#include "Timer.h"
#include <iostream>


Timer::Timer(const std::string& msg)
{
    mStart = std::clock();
    std::cout << msg << std::endl;
}

long double Timer::Stop(const std::string& msg)
{
    std::clock_t stop = std::clock();
    long double time_elapsed_s = long double(stop-mStart) / CLOCKS_PER_SEC;
    long double time_elapsed_ms = time_elapsed_s*1000.0;
    std::cout << msg << time_elapsed_s << " s " << "(" << time_elapsed_ms << "ms)" << std::endl;
    return time_elapsed_ms;
}

void Timer::Restart(const std::string& msg)
{
    mStart = std::clock();
    std::cout << msg << std::endl;
}


#include <GL/glew.h>
bool GpuTimer::DisableAll = false;

GpuTimer::GpuTimer(const std::string& msg)
{
   if(DisableAll==true) return;
   if(glGenQueries != nullptr)
   {
      glGenQueries(2, mQuery);
      std::cout << msg << std::endl;
      glQueryCounter(mQuery[0], GL_TIMESTAMP);
   }
}

long double GpuTimer::Stop(const std::string& msg)
{
    if(DisableAll==true) return -1.0;

    glQueryCounter(mQuery[1], GL_TIMESTAMP);
    GLint stopTimerAvailable = 0;
    while (!stopTimerAvailable) 
    {
        glGetQueryObjectiv(mQuery[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
    }

    GLuint64 startTime, stopTime;
    glGetQueryObjectui64v(mQuery[0], GL_QUERY_RESULT, &startTime);
    glGetQueryObjectui64v(mQuery[1], GL_QUERY_RESULT, &stopTime);

    long double time_elapsed_ms = long double(stopTime - startTime) / 1000000.0;
    long double time_elapsed_s = time_elapsed_ms / 1000.0;

    std::cout << msg << time_elapsed_s << " s " << "(" << time_elapsed_ms << "ms)" << std::endl;
    return time_elapsed_ms;
}

void GpuTimer::Restart(const std::string& msg)
{
   if(DisableAll==true) return;
   
   if(mQuery[0] == -1)
   {
      glGenQueries(2, mQuery);
   }
   glQueryCounter(mQuery[0], GL_TIMESTAMP);
   std::cout << msg << std::endl;
}