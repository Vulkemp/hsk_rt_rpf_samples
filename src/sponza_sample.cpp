#include "sponza_sample.hpp"
#include <gltfconvert/hsk_modelconverter.hpp>
#include <scenegraph/components/hsk_camera.hpp>
#include <scenegraph/components/hsk_freecameracontroller.hpp>
#include <imgui/imgui.h>
#include <memory/hsk_managedimage.hpp>
#include <vulkan/vulkan.h>

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

void ImportanceSamplingRtProject::OnEvent(const hsk::Event *event)
{
    auto buttonInput = dynamic_cast<const hsk::EventInputBinary *>(event);
    auto axisInput = dynamic_cast<const hsk::EventInputAnalogue *>(event);
    auto windowResized = dynamic_cast<const hsk::EventWindowResized *>(event);
    if (windowResized)
    {
        spdlog::info("Window resized w {} h {}", windowResized->Current.Width, windowResized->Current.Height);
    }
    mScene->InvokeOnEvent(event);
    // mFreeFlightCameraController.OnEvent(event);

    // process events for imgui
    mImguiStage.ProcessSdlEvent(&(event->RawSdlEventData));
}

void ImportanceSamplingRtProject::loadScene()
{

    mScene = std::make_unique<hsk::Scene>(&mContext);
    {
        // std::string fullFileName = hsk::MakeRelativePath("models/minimal.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/BoomBoxWithAxes/glTF/BoomBoxWithAxes.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("../sponza_model/Main/NewSponza_Main_Blender_glTF.gltf");
        std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");
        hsk::ModelConverter converter(mScene.get());
        converter.LoadGltfModel(fullFileName);
    }
    {
        // std::string fullFileName = hsk::MakeRelativePath("../sponza_model/PKG_B_Ivy/NewSponza_IvyGrowth_glTF.gltf");
        std::string fullFileName = hsk::MakeRelativePath("../glTF-Sample-Models/2.0/InterpolationTest/glTF/InterpolationTest.gltf");
        hsk::ModelConverter converter(mScene.get());
        // std::string fullFileName = hsk::MakeRelativePath("../sponza_model/PKG_B_Ivy/NewSponza_IvyGrowth_glTF.gltf");

        converter.LoadGltfModel(fullFileName);
    }
    mScene->CreateTlas();

    auto cameraNode = mScene->MakeNode();

    cameraNode->MakeComponent<hsk::Camera>()->InitDefault();
    cameraNode->MakeComponent<hsk::FreeCameraController>();
}

void ImportanceSamplingRtProject::Cleanup()
{
    vkDeviceWaitIdle(mContext.Device);
    mScene->Cleanup();
    mScene = nullptr;
    mGbufferStage.Destroy();
    mImguiStage.Destroy();
    // mFlipImageStage.Destroy();
    mRaytraycingStage.Destroy();

    DefaultAppBase::Cleanup();
}

void ImportanceSamplingRtProject::PrepareImguiWindow()
{
    mImguiStage.AddWindowDraw([this]()
                              {
		    ImGui::Begin("window");
		    ImGui::Text("FPS: %f", mFps);
			ImGui::End(); });
}

void ImportanceSamplingRtProject::ConfigureStages()
{
    mGbufferStage.Init(&mContext, mScene.get());
    auto albedoImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);

    mRaytraycingStage.Init(&mContext, mScene.get());
    auto rtImage = mRaytraycingStage.GetColorAttachmentByName(hsk::RaytracingStage::RaytracingRenderTargetName);

    // init flip image stage
    // mFlipImageStage.Init(&mContext, albedoImage);

    // init imgui
    // auto flippedImage = mFlipImageStage.GetColorAttachmentByName(hsk::FlipImageStage::FlipTarget);
    mImguiStage.Init(&mContext, rtImage);
    PrepareImguiWindow();

    // �nit copy stage
    mImageToSwapchainStage.Init(&mContext, rtImage, hsk::ImageToSwapchainStage::PostCopy{.AccessFlags=(VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT),.ImageLayout=(VkImageLayout::VK_IMAGE_LAYOUT_GENERAL), .QueueFamilyIndex=(mContext.QueueGraphics)});
}

void ImportanceSamplingRtProject::RecordCommandBuffer(hsk::FrameRenderInfo &renderInfo)
{
    mScene->Update(renderInfo);
    mGbufferStage.RecordFrame(renderInfo);

    mRaytraycingStage.RecordFrame(renderInfo);

    // flip opengl coordinate system to vulkan
    // mFlipImageStage.RecordFrame(renderInfo);

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
    // mFlipImageStage.OnResized(size, albedoImage);
    // auto flippedImage = mFlipImageStage.GetColorAttachmentByName(hsk::FlipImageStage::FlipTarget);
    mRaytraycingStage.OnResized(size);
    auto rtImage = mRaytraycingStage.GetColorAttachmentByName(hsk::RaytracingStage::RaytracingRenderTargetName);

    mImguiStage.OnResized(size, rtImage);
    mImageToSwapchainStage.OnResized(size, rtImage);
}
