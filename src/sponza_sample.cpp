#include "sponza_sample.hpp"
#include <gltfconvert/hsk_modelconverter.hpp>
#include <scenegraph/components/hsk_camera.hpp>
#include <scenegraph/components/hsk_freecameracontroller.hpp>
#include <imgui/imgui.h>
#include <memory/hsk_managedimage.hpp>

void ImportanceSamplingRtProject::Init()
{
    hsk::logger()->set_level(spdlog::level::debug);
    mDisplayConfig.Window.DisplayMode(hsk::EDisplayMode::WindowedResizable);
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
    vkDeviceWaitIdle(mContext.Device);
    mScene->Cleanup();
    mScene = nullptr;
    mGbufferStage.Destroy();
    DefaultAppBase::Cleanup();
}

void ImportanceSamplingRtProject::RecordImguiWindow(hsk::FrameRenderInfo& renderInfo)
{
    mImguiStage.AddWindowDraw([]() {
		    ImGui::Begin("window");
		    ImGui::Text("FPS: %f", 0);
			ImGui::End();
        });
}

void ImportanceSamplingRtProject::ConfigureStages()
{
    mGbufferStage.Init(&mContext, mScene.get());
    mSwapchainCopySourceImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);

    // init imgui
    mImguiStage.Init(&mContext, mSwapchainCopySourceImage);
}

void ImportanceSamplingRtProject::RecordCommandBuffer(hsk::FrameRenderInfo &renderInfo)
{
    mScene->Update(renderInfo);
    mGbufferStage.RecordFrame(renderInfo);
    
    //VkImageSubresourceRange range{};
    //range.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    //range.baseMipLevel = 0;
    //range.levelCount = VK_REMAINING_MIP_LEVELS;
    //range.baseArrayLayer = 0;
    //range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    //hsk::ManagedImage::LayoutTransitionInfo layoutTransitionInfo;
    //layoutTransitionInfo.CommandBuffer = renderInfo.GetCommandBuffer();
    //layoutTransitionInfo.BarrierSrcAccessMask = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
    //layoutTransitionInfo.BarrierDstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
    //layoutTransitionInfo.NewImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
    //layoutTransitionInfo.OldImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
    //layoutTransitionInfo.SrcQueueFamilyIndex = mDefaultQueue.QueueFamilyIndex;
    //layoutTransitionInfo.DstQueueFamilyIndex = mDefaultQueue.QueueFamilyIndex;
    //layoutTransitionInfo.SubresourceRange = range;
    //layoutTransitionInfo.SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;  // TODO: these are wrong most likely
    //layoutTransitionInfo.DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    //mSwapchainCopySourceImage->TransitionLayout(layoutTransitionInfo);


    //// Copy one of the g-buffer images into the swapchain / TODO: This is not done
    //VkImageSubresourceLayers layers = {};
    //layers.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    //layers.mipLevel = 0;
    //layers.baseArrayLayer = 0;
    //layers.layerCount = 1;

    //VkImageBlit blitRegion = {};
    //blitRegion.srcSubresource = layers;
    //blitRegion.srcOffsets[0] = {};
    //blitRegion.srcOffsets[1] = VkOffset3D{ .x = (int32_t)mDisplayConfig.SwapchainVkb.extent.width, .y = (int32_t)mDisplayConfig.SwapchainVkb.extent.height, .z = 1 };
    //blitRegion.dstSubresource = layers;
    //blitRegion.dstOffsets[1] = { .z = 1 };
    //blitRegion.dstOffsets[0] = VkOffset3D{ .x = (int32_t)mDisplayConfig.SwapchainVkb.extent.width, .y = (int32_t)mDisplayConfig.SwapchainVkb.extent.height, .z = 0 };

    //vkCmdBlitImage(renderInfo.GetCommandBuffer(), mSwapchainCopySourceImage->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
    //    mDisplayConfig.SwapchainImages[renderInfo.GetSwapchainImageIndex()].Image, VkImageLayout::VK_IMAGE_LAYOUT_GENERAL, 1, &blitRegion,
    //    VkFilter::VK_FILTER_NEAREST);

    RecordImguiWindow(renderInfo);
    mImguiStage.RecordFrame(renderInfo);
}

void ImportanceSamplingRtProject::OnResized(VkExtent2D size)
{
    mScene->InvokeOnResized(size);
    mGbufferStage.OnResized(size);
    mSwapchainCopySourceImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);
    mImguiStage.OnResized(size, mSwapchainCopySourceImage);
}
