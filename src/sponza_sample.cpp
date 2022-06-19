#include "sponza_sample.hpp"
#include <gltfconvert/hsk_modelconverter.hpp>
#include <scenegraph/components/hsk_camera.hpp>
#include <scenegraph/components/hsk_freecameracontroller.hpp>

void ImportanceSamplingRtProject::Init()
{
    hsk::logger()->set_level(spdlog::level::debug);
    mWindow.DisplayMode(hsk::EDisplayMode::WindowedResizable);
    loadScene();
    ConfigureStages();
}

void ImportanceSamplingRtProject::Update(float delta)
{
    // mFreeFlightCameraController.Update(delta);
}

void ImportanceSamplingRtProject::OnEvent(hsk::Event::ptr event)
{
    auto buttonInput = std::dynamic_pointer_cast<hsk::EventInputBinary>(event);
    auto axisInput = std::dynamic_pointer_cast<hsk::EventInputAnalogue>(event);
    auto windowResized = std::dynamic_pointer_cast<hsk::EventWindowResized>(event);
    if (windowResized)
    {
        spdlog::info("Window resized w {} h {}", windowResized->Current().Width, windowResized->Current().Height);
    }
    mScene->InvokeOnEvent(event);
    // mFreeFlightCameraController.OnEvent(event);
}

void ImportanceSamplingRtProject::loadScene()
{

    mScene = std::make_unique<hsk::Scene>(&mContext);
    {
        // std::string fullFileName = hsk::MakeRelativePath("models/minimal.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/GearboxAssy/glTF/GearboxAssy.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("../sponza_model/Main/NewSponza_Main_Blender_glTF.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("../sponza_model/PKG_B_Ivy/NewSponza_IvyGrowth_glTF.gltf");
        std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");
        hsk::ModelConverter converter(mScene.get());
        converter.LoadGltfModel(fullFileName);
    }
    {
        std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/Avocado/glTF/Avocado.gltf");
        hsk::ModelConverter converter(mScene.get());
        // converter.LoadGltfModel(fullFileName);
    }

    auto cameraNode = mScene->MakeNode();

    cameraNode->MakeComponent<hsk::Camera>()->InitDefault();
    cameraNode->MakeComponent<hsk::FreeCameraController>();

    // propagate vk variables
    // mScene.Context(&mContext);
    // mScene.LoadFromFile(fullFileName);

    // if (mScene.GetCameras().size() == 0)
    // {
    //     throw hsk::Exception("Scene has no camera attached!");
    // }
    // mFreeFlightCameraController.Init(&mContext, mScene.GetCameras()[0].get(), &mOsManager);
}

void ImportanceSamplingRtProject::Cleanup()
{
    vkDeviceWaitIdle(mDevice);
    mScene->Cleanup();
    mScene = nullptr;
    mGbufferStage.Destroy();
    DefaultAppBase::Cleanup();
}

void ImportanceSamplingRtProject::ConfigureStages()
{
    mGbufferStage.Init(&mContext, mScene.get());
    mSwapchainCopySourceImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);
}

void ImportanceSamplingRtProject::RecordCommandBuffer(hsk::FrameRenderInfo &renderInfo)
{
    mScene->Update(renderInfo);
    mGbufferStage.RecordFrame(renderInfo);
}

void ImportanceSamplingRtProject::OnResized(VkExtent2D size)
{
    mScene->InvokeOnResized(size);
    mGbufferStage.OnResized(size);
    mSwapchainCopySourceImage = mGbufferStage.GetColorAttachmentByName("Normal");
}
