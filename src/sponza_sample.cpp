#include "sponza_sample.hpp"
#include <bench/foray_hostbenchmark.hpp>
#include <core/foray_managedimage.hpp>
#include <gltf/foray_modelconverter.hpp>
#include <imgui/imgui.h>
#include <scene/components/foray_camera.hpp>
#include <scene/components/foray_freecameracontroller.hpp>
#include <scene/globalcomponents/foray_cameramanager.hpp>
#include <scene/globalcomponents/foray_tlasmanager.hpp>
#include <util/foray_imageloader.hpp>

void ImportanceSamplingRtProject::ApiBeforeInit()
{
    mAuxiliaryCommandBufferCount = 1;
    mWindowSwapchain.GetWindow().DisplayMode(foray::osi::EDisplayMode::WindowedResizable);
}

void ImportanceSamplingRtProject::ApiInit()
{
    foray::logger()->set_level(spdlog::level::debug);
    LoadEnvironmentMap();
    GenerateNoiseSource();
    loadScene();
    ConfigureStages();
}

void ImportanceSamplingRtProject::ApiBeforeDeviceSelection(vkb::PhysicalDeviceSelector& pds)
{
    pds.add_required_extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
#ifdef WIN32
    pds.add_required_extension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    pds.add_required_extension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#else
    pds.add_required_extension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    pds.add_required_extension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
#endif
}

void ImportanceSamplingRtProject::ApiBeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder)
{
    mTimelineFeature =
        VkPhysicalDeviceTimelineSemaphoreFeatures{.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES, .timelineSemaphore = VK_TRUE};

    deviceBuilder.add_pNext(&mTimelineFeature);
}

void ImportanceSamplingRtProject::ApiOnEvent(const foray::osi::Event* event)
{
    mScene->InvokeOnEvent(event);

    // process events for imgui
    mImguiStage.ProcessSdlEvent(&(event->RawSdlEventData));
}

void ImportanceSamplingRtProject::loadScene()
{
    std::vector<std::string> scenePaths({
        // "models/minimal.gltf",
        // "../glTF-Sample-Models/2.0/BoomBoxWithAxes/glTF/BoomBoxWithAxes.gltf",
        // "../sponza_model/Main/NewSponza_Main_Blender_glTF.gltf",
        // "../sponza_model/PKG_B_Ivy/NewSponza_IvyGrowth_glTF.gltf",
        "../glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf",
        // "../glTF-Sample-Models/2.0/InterpolationTest/glTF/InterpolationTest.gltf",
        // "../../foray-examples/data/gltf/minimal/minimal.gltf"
    });

    mScene = std::make_unique<foray::scene::Scene>(&mContext);
    foray::gltf::ModelConverter converter(mScene.get());
    for(const auto& path : scenePaths)
    {
        converter.LoadGltfModel(foray::osi::MakeRelativePath(path));
    }
    mScene->MakeComponent<foray::scene::TlasManager>(&mContext)->CreateOrUpdate();

    auto cameraNode = mScene->MakeNode();

    cameraNode->MakeComponent<foray::scene::Camera>()->InitDefault();
    cameraNode->MakeComponent<foray::scene::FreeCameraController>();
    mScene->GetComponent<foray::scene::CameraManager>()->RefreshCameraList();

    for(int32_t i = 0; i < scenePaths.size(); i++)
    {
        const auto& path = scenePaths[i];
        const auto& log  = converter.GetBenchmark().GetLogs()[i];
        foray::logger()->info("Model Load \"{}\":\n{}", path, log.PrintPretty());
    }
}

void ImportanceSamplingRtProject::LoadEnvironmentMap()
{

    constexpr VkFormat                    hdrVkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    foray::util::ImageLoader<hdrVkFormat> imageLoader;
    // env maps at https://polyhaven.com/a/alps_field
    std::string pathToEnvMap = std::string(foray::osi::CurrentWorkingDirectory()) + "/../data/textures/envmap.exr";
    if(!imageLoader.Init(pathToEnvMap))
    {
        foray::logger()->warn("Loading env map failed \"{}\"", pathToEnvMap);
        return;
    }
    if(!imageLoader.Load())
    {
        foray::logger()->warn("Loading env map failed #2 \"{}\"", pathToEnvMap);
        return;
    }

    VkExtent3D ext3D{
        .width  = imageLoader.GetInfo().Extent.width,
        .height = imageLoader.GetInfo().Extent.height,
        .depth  = 1,
    };

    foray::core::ManagedImage::CreateInfo ci("Environment map", VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, hdrVkFormat,
                                             ext3D);

    imageLoader.InitManagedImage(&mContext, &mSphericalEnvMap, ci);
    imageLoader.Destroy();
}

void ImportanceSamplingRtProject::GenerateNoiseSource()
{
    foray::bench::HostBenchmark bench;
    bench.Begin();
    mNoiseSource.Create(&mContext);
    bench.End();
    foray::logger()->info("Create Noise Tex \n{}", bench.GetLogs().front().PrintPretty());
}

void ImportanceSamplingRtProject::ApiDestroy()
{
    mNoiseSource.Destroy();
    mScene->Destroy();
    mScene = nullptr;
    mGbufferStage.Destroy();
    mImguiStage.Destroy();
    mRaytraycingStage.Destroy();
    mSphericalEnvMap.Destroy();
    mDenoiser.Destroy();
    mDenoiseSemaphore.Destroy();
    mDenoisedImage.Destroy();
}

void ImportanceSamplingRtProject::PrepareImguiWindow()
{
    mImguiStage.AddWindowDraw([this]() {
        foray::base::RenderLoop::FrameTimeAnalysis analysis = this->GetRenderLoop().AnalyseFrameTimes();

        ImGui::Begin("window");
        if(analysis.Count > 0)
        {
            ImGui::Text("FPS: %f avg %f min", 1.f / analysis.AvgFrameTime, 1.f / analysis.MaxFrameTime);
        }

        const char* current = mCurrentOutput.data();
        if(ImGui::BeginCombo("Output", current))
        {
            std::string_view newOutput = mCurrentOutput;
            for(auto output : mOutputs)
            {
                bool selected = output.first == mCurrentOutput;
                if(ImGui::Selectable(output.first.data(), selected))
                {
                    newOutput = output.first;
                }
            }

            if(newOutput != mCurrentOutput)
            {
                mCurrentOutput = newOutput;
                mOutputChanged = true;
            }

            ImGui::EndCombo();
        }

#ifdef ENABLE_GBUFFER_BENCH
        if(mDisplayedLog.Timestamps.size() > 0 && ImGui::CollapsingHeader("GBuffer Benchmark"))
        {
            mDisplayedLog.PrintImGui();
        }
#endif  // ENABLE_GBUFFER_BENCH

        ImGui::End();
    });
}

void ImportanceSamplingRtProject::ConfigureStages()
{
    mGbufferStage.Init(&mContext, mScene.get());
    auto albedoImage = mGbufferStage.GetColorAttachmentByName(foray::stages::GBufferStage::Albedo);
    auto normalImage = mGbufferStage.GetColorAttachmentByName(foray::stages::GBufferStage::WorldspaceNormal);
    auto motionImage = mGbufferStage.GetColorAttachmentByName(foray::stages::GBufferStage::MotionVector);

    mRaytraycingStage.Init(&mContext, mScene.get(), &mSphericalEnvMap, &mNoiseSource.GetImage());
    auto rtImage = mRaytraycingStage.GetColorAttachmentByName(foray::stages::RaytracingStage::RaytracingRenderTargetName);

    mDenoiseSemaphore.Create(&mContext);

    VkExtent3D extent{.width = mContext.GetSwapchainSize().width, .height = mContext.GetSwapchainSize().height, .depth = 1};

    foray::core::ManagedImage::CreateInfo ci("Denoised Image",
                                             VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                 | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                             VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, extent);

    mDenoisedImage.Create(&mContext, ci);

    foray::stages::DenoiserConfig config(rtImage, &mDenoisedImage);
    config.AlbedoInput = albedoImage;
    config.NormalInput = normalImage;
    config.MotionInput = motionImage;
    config.Semaphore   = &mDenoiseSemaphore;

    mDenoiser.Init(&mContext, config);

    UpdateOutputs();

    mImguiStage.Init(&mContext, mOutputs[mCurrentOutput]);
    PrepareImguiWindow();

    // �nit copy stage
    mImageToSwapchainStage.Init(&mContext, mOutputs[mCurrentOutput]);

    // Setup Inflight Frames
    for(foray::base::InFlightFrame& frame : mInFlightFrames)
    {
        frame.GetAuxiliaryCommandBuffer(0).AddSignalSemaphore(foray::core::SemaphoreSubmit::Timeline(mDenoiseSemaphore.GetSemaphore(), 0, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT));
        frame.GetPrimaryCommandBuffer().AddWaitSemaphore(foray::core::SemaphoreSubmit::Binary(frame.GetSwapchainImageReady(), VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR));
        frame.GetPrimaryCommandBuffer().AddWaitSemaphore(foray::core::SemaphoreSubmit::Timeline(mDenoiseSemaphore.GetSemaphore(), 0, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT));
    }

    RegisterRenderStage(&mGbufferStage);
    RegisterRenderStage(&mRaytraycingStage);
    RegisterRenderStage(&mDenoiser);
    RegisterRenderStage(&mImguiStage);
    RegisterRenderStage(&mImageToSwapchainStage);
}

void ImportanceSamplingRtProject::ApiRender(foray::base::FrameRenderInfo& renderInfo)
{
    if(mOutputChanged)
    {
        ApplyOutput();
        mOutputChanged = false;
    }

    uint64_t timelineValueAtBegin       = renderInfo.GetFrameNumber() * 2;
    uint64_t timelineValueBeforeDenoise = timelineValueAtBegin + 1;
    uint64_t timelineValueAfterDenoise  = timelineValueBeforeDenoise + 1;

    foray::core::DeviceCommandBuffer& primaryCmdBuffer = renderInfo.GetPrimaryCommandBuffer();
    foray::core::DeviceCommandBuffer& auxCmdBuffer     = renderInfo.GetAuxCommandBuffer(0);

    // Update timeline synchronisation semaphores
    auxCmdBuffer.GetSignalSemaphores().back().TimelineValue   = timelineValueBeforeDenoise;
    primaryCmdBuffer.GetWaitSemaphores().back().TimelineValue = timelineValueAfterDenoise;

    // Begin aux command buffer
    auxCmdBuffer.Begin();

    mScene->Update(renderInfo, auxCmdBuffer);
    mGbufferStage.RecordFrame(auxCmdBuffer, renderInfo);

    mRaytraycingStage.RecordFrame(auxCmdBuffer, renderInfo);

    mDenoiser.BeforeDenoise(auxCmdBuffer, renderInfo);
    auxCmdBuffer.End();
    auxCmdBuffer.Submit();

    mDenoiser.DispatchDenoise(timelineValueBeforeDenoise, timelineValueAfterDenoise);

    primaryCmdBuffer.Begin();

    mDenoiser.AfterDenoise(primaryCmdBuffer, renderInfo);

    // draw imgui windows
    mImguiStage.RecordFrame(primaryCmdBuffer, renderInfo);

    renderInfo.GetInFlightFrame()->ClearSwapchainImage(primaryCmdBuffer, renderInfo.GetImageLayoutCache());

    // copy final image to swapchain
    mImageToSwapchainStage.RecordFrame(primaryCmdBuffer, renderInfo);

    renderInfo.GetInFlightFrame()->PrepareSwapchainImageForPresent(primaryCmdBuffer, renderInfo.GetImageLayoutCache());
    primaryCmdBuffer.End();
    primaryCmdBuffer.Submit();
}

void ImportanceSamplingRtProject::ApiQueryResultsAvailable(uint64_t frameIndex)
{
#ifdef ENABLE_GBUFFER_BENCH
    mGbufferStage.GetBenchmark().LogQueryResults(frameIndex);
    mDisplayedLog = mGbufferStage.GetBenchmark().GetLogs().back();
#endif  // ENABLE_GBUFFER_BENCH
}

void ImportanceSamplingRtProject::ApiOnResized(VkExtent2D size)
{
    mScene->InvokeOnResized(size);

    mDenoisedImage.Resize(VkExtent3D{size.width, size.height, 1});
}

void lUpdateOutput(std::unordered_map<std::string_view, foray::core::ManagedImage*>& map, foray::stages::RenderStage& stage, const std::string_view name)
{
    map[name] = stage.GetColorAttachmentByName(name);
}

void ImportanceSamplingRtProject::UpdateOutputs()
{
    mOutputs.clear();
    lUpdateOutput(mOutputs, mGbufferStage, foray::stages::GBufferStage::Albedo);
    lUpdateOutput(mOutputs, mGbufferStage, foray::stages::GBufferStage::WorldspacePosition);
    lUpdateOutput(mOutputs, mGbufferStage, foray::stages::GBufferStage::WorldspaceNormal);
    lUpdateOutput(mOutputs, mRaytraycingStage, foray::stages::RaytracingStage::RaytracingRenderTargetName);
    mOutputs.emplace("Denoised Image", &mDenoisedImage);

    if(mCurrentOutput.size() == 0 || !mOutputs.contains(mCurrentOutput))
    {
        if(mOutputs.size() == 0)
        {
            mCurrentOutput = "";
        }
        else
        {
            mCurrentOutput = mOutputs.begin()->first;
        }
    }
}

void ImportanceSamplingRtProject::ApplyOutput()
{
    vkDeviceWaitIdle(mDevice);
    auto output = mOutputs[mCurrentOutput];
    mImguiStage.SetTargetImage(output);
    mImageToSwapchainStage.SetTargetImage(output);
}