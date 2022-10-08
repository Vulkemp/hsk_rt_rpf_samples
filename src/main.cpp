#include "sponza_sample.hpp"
#include "configurepath.cmakegenerated.hpp"
#include <osi/foray_env.hpp>

int main(int argv, char** args)
{
    foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE_PATH);
    ImportanceSamplingRtProject project;
    return project.Run();
}