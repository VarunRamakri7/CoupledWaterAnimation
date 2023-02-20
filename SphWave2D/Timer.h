#pragma once
#include <ctime>
#include <string>

class Timer
{
    private:
        std::clock_t mStart;


    public:
        Timer(const std::string& msg = "");
        virtual long double  Stop(const std::string& msg = "");
        virtual void Restart(const std::string& msg = "");
};


class GpuTimer:public Timer
{
    private:
        unsigned int mQuery[2] = {-1U, -1U};


    public:
        GpuTimer(const std::string& msg = "");
        long double Stop(const std::string& msg = "") override;
        void Restart(const std::string& msg = "") override;

   static bool DisableAll;
};