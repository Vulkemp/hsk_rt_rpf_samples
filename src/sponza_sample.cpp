#include "sponza_sample.hpp"

void ImportanceSamplingRtProject::Init()
{
    mWindow.DisplayMode(hsk::EDisplayMode::WindowedResizable);
    loadScene();
    ConfigureStages();
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

    void ImportanceSamplingRtProject::loadScene()
    {
        // std::string fullFileName = hsk::MakeRelativePath("models/minimal.gltf");
        std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/Avocado/glTF/Avocado.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("sponza_model/glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");

        // propagate vk variables
        mScene.Context(&mContext);

        mScene.LoadFromFile(fullFileName);
    }

    void ImportanceSamplingRtProject::Cleanup() {
        mScene.Cleanup();
        DefaultAppBase::Cleanup();
    }

    void ImportanceSamplingRtProject::ConfigureStages(){
        mGbufferStage.Init(&mContext, &mScene);
    }

