#include "sponza_sample.hpp"
#include "configurepath.cmakegenerated.hpp"
#include <hsk_env.hpp>

int main(int argv, char** args)
{
    hsk::OverrideCurrentWorkingDirectory(CWD_OVERRIDE_PATH);
    ImportanceSamplingRtProject project;
    return project.Run();
}