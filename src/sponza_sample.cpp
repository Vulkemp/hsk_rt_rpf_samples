#include "sponza_sample.hpp"
#include <gltfconvert/hsk_modelconverter.hpp>

void ImportanceSamplingRtProject::Init()
{
    mWindow.DisplayMode(hsk::EDisplayMode::WindowedResizable);
    loadScene();
    ConfigureStages();
}

void ImportanceSamplingRtProject::Update(float delta)
{
    mFreeFlightCameraController.Update(delta);
}

void ImportanceSamplingRtProject::OnEvent(hsk::Event::ptr event)
{
    auto buttonInput = std::dynamic_pointer_cast<hsk::EventInputBinary>(event);
    auto axisInput = std::dynamic_pointer_cast<hsk::EventInputAnalogue>(event);
    auto windowResized = std::dynamic_pointer_cast<hsk::EventWindowResized>(event);
    if (buttonInput)
    {
        spdlog::info("Device \"{}\" Button {} - {}", buttonInput->Device()->Name(), buttonInput->Button()->Name(), buttonInput->Pressed() ? "pressed" : "released");
    }
    if (axisInput)
    {
        spdlog::info("Device \"{}\" Axis {} - {}", axisInput->Device()->Name(), axisInput->Axis()->Name(), axisInput->State());
    }
    if (windowResized)
    {
        spdlog::info("Window resized w {} h {}", windowResized->Current().Width, windowResized->Current().Height);
    }
    mFreeFlightCameraController.OnEvent(event);
}

void ImportanceSamplingRtProject::OnResized(VkExtent2D size)
{
    mGbufferStage.OnResized(size);
}

void ImportanceSamplingRtProject::loadScene()
{
    // std::string fullFileName = hsk::MakeRelativePath("models/minimal.gltf");
    // std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/Avocado/glTF/Avocado.gltf");
    std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");

    // {
    //     hsk::NScene scene(&mContext);
    //     hsk::ModelConverter converter(&scene);
    //     converter.LoadGltfModel(fullFileName);
    // }

    // propagate vk variables
    mScene.Context(&mContext);
    mScene.LoadFromFile(fullFileName);
    if (mScene.GetCameras().size() == 0)
    {
        throw hsk::Exception("Scene has no camera attached!");
    }
    mFreeFlightCameraController.Init(&mContext, mScene.GetCameras()[0].get(), &mOsManager);
}

void ImportanceSamplingRtProject::Cleanup()
{
    vkDeviceWaitIdle(mDevice);
    mScene.Cleanup();
    mGbufferStage.Destroy();
    DefaultAppBase::Cleanup();
}

void ImportanceSamplingRtProject::ConfigureStages()
{
    mGbufferStage.Init(&mContext, &mScene);
    mSwapchainCopySourceImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);
}

void ImportanceSamplingRtProject::RecordCommandBuffer(hsk::FrameRenderInfo &renderInfo)
{
    mGbufferStage.RecordFrame(renderInfo);
}
