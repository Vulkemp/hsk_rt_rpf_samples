#include "sponza_sample.hpp"

void ImportanceSamplingRtProject::Init()
{
    mWindow.DisplayMode(hsk::EDisplayMode::WindowedResizable);
    initVulkan();
}

void ImportanceSamplingRtProject::OnEvent(hsk::Event::ptr event)
{
    auto buttonInput = std::dynamic_pointer_cast<hsk::EventInputBinary>(event);
    auto axisInput   = std::dynamic_pointer_cast<hsk::EventInputAnalogue>(event);
    if(buttonInput)
    {
        spdlog::info("Device \"{}\" Button {} - {}", buttonInput->Device()->Name(), buttonInput->Button()->Name(), buttonInput->Pressed() ? "pressed" : "released");
    }
    if(axisInput)
    {
        spdlog::info("Device \"{}\" Axis {} - {}", axisInput->Device()->Name(), axisInput->Axis()->Name(), axisInput->State());
    }
}
