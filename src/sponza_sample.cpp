#include "sponza_sample.hpp"
#include <gltfconvert/hsk_modelconverter.hpp>
#include <scenegraph/components/hsk_camera.hpp>
#include <scenegraph/components/hsk_freecameracontroller.hpp>
#include <imgui/imgui.h>
#include <memory/hsk_managedimage.hpp>

void ImportanceSamplingRtProject::Init()
{
    hsk::logger()->set_level(spdlog::level::debug);
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

    // process events for imgui
    mImguiStage.ProcessSdlEvent(event->GetRawSdlEvent());
}

void ImportanceSamplingRtProject::loadScene()
{

    mScene = std::make_unique<hsk::Scene>(&mContext);
    {
        // std::string fullFileName = hsk::MakeRelativePath("models/minimal.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/GearboxAssy/glTF/GearboxAssy.gltf");
        //std::string fullFileName = hsk::MakeRelativePath("../sponza_model/Main/NewSponza_Main_Blender_glTF.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("../sponza_model/PKG_B_Ivy/NewSponza_IvyGrowth_glTF.gltf");
        std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");
        hsk::ModelConverter converter(mScene.get());
        converter.LoadGltfModel(fullFileName);
        mScene->CreateTlas();
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
    vkDeviceWaitIdle(mContext.Device);
    mScene->Cleanup();
    mScene = nullptr;
    mGbufferStage.Destroy();
    mImguiStage.Destroy();
    mFlipImageStage.Destroy();
    mRaytraycingStage.Destroy();

    DefaultAppBase::Cleanup();
}

void ImportanceSamplingRtProject::PrepareImguiWindow()
{
    mImguiStage.AddWindowDraw([this]() {
		    ImGui::Begin("window");
		    ImGui::Text("FPS: %f", mFps);
			ImGui::End();
        });
}

void ImportanceSamplingRtProject::ConfigureStages()
{
    mGbufferStage.Init(&mContext, mScene.get());
    auto albedoImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);

    mRaytraycingStage.Init(&mContext, mScene.get());

    // init flip image stage
    mFlipImageStage.Init(&mContext, albedoImage);

    // init imgui
    auto flippedImage = mFlipImageStage.GetColorAttachmentByName(hsk::FlipImageStage::FlipTarget);
    mImguiStage.Init(&mContext, flippedImage);
    PrepareImguiWindow();
 
    // ínit copy stage
    mImageToSwapchainStage.Init(&mContext, flippedImage);
}

void ImportanceSamplingRtProject::RecordCommandBuffer(hsk::FrameRenderInfo &renderInfo)
{
    mScene->Update(renderInfo);
    mGbufferStage.RecordFrame(renderInfo);

    mRaytraycingStage.RecordFrame(renderInfo);

    // flip opengl coordinate system to vulkan
    mFlipImageStage.RecordFrame(renderInfo);

    // draw imgui windows
    mImguiStage.RecordFrame(renderInfo);

    // copy final image to swapchain
    mImageToSwapchainStage.RecordFrame(renderInfo);
}

void ImportanceSamplingRtProject::OnResized(VkExtent2D size)
{
    mScene->InvokeOnResized(size);
    mGbufferStage.OnResized(size);
    auto albedoImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);
    mFlipImageStage.OnResized(size, albedoImage);

    auto flippedImage = mFlipImageStage.GetColorAttachmentByName(hsk::FlipImageStage::FlipTarget);
    mImguiStage.OnResized(size, flippedImage);
    mImageToSwapchainStage.OnResized(size, flippedImage);
}
