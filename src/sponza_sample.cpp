#include "sponza_sample.hpp"
#include <gltfconvert/hsk_modelconverter.hpp>
#include <scenegraph/components/hsk_camera.hpp>
#include <scenegraph/globalcomponents/hsk_cameramanager.hpp>
#include <scenegraph/globalcomponents/hsk_tlasmanager.hpp>
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
    if (mOutputChanged)
    {
        ApplyOutput();
        mOutputChanged = false;
    }
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
    mScene->MakeComponent<hsk::TlasManager>(&mContext)->CreateOrUpdate();

    auto cameraNode = mScene->MakeNode();

    cameraNode->MakeComponent<hsk::Camera>()->InitDefault();
    cameraNode->MakeComponent<hsk::FreeCameraController>();
    mScene->GetComponent<hsk::CameraManager>()->RefreshCameraList();
}

void ImportanceSamplingRtProject::Destroy()
{
    vkDeviceWaitIdle(mContext.Device);
    mScene->Destroy();
    mScene = nullptr;
    mGbufferStage.Destroy();
    mImguiStage.Destroy();
    mRaytraycingStage.Destroy();

    DefaultAppBase::Destroy();
}

void ImportanceSamplingRtProject::PrepareImguiWindow()
{
    mImguiStage.AddWindowDraw([this]()
                              {
		    ImGui::Begin("window");
		    ImGui::Text("FPS: %f", mFps);

            const char* current = mCurrentOutput.data();
            if (ImGui::BeginCombo("Output", current))
            {
                std::string_view newOutput = mCurrentOutput;
                for (auto output : mOutputs)
                {
                    bool selected = output.first == mCurrentOutput;
                    if (ImGui::Selectable(output.first.data(), selected))
                    {
                        newOutput = output.first;
                    }
                }

                if (newOutput != mCurrentOutput)
                {
                    mCurrentOutput = newOutput;
                    mOutputChanged = true;
                }

                ImGui::EndCombo();
            }

#ifdef ENABLE_GBUFFER_BENCH
            if (mDisplayedLog.Timestamps.size() > 0 && ImGui::CollapsingHeader("GBuffer Benchmark"))
            {
                mDisplayedLog.PrintImGui();
            }
#endif // ENABLE_GBUFFER_BENCH

			ImGui::End(); });
}

void ImportanceSamplingRtProject::ConfigureStages()
{
    mGbufferStage.Init(&mContext, mScene.get());
    auto albedoImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::Albedo);
    auto normalImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::WorldspaceNormal);

    mRaytraycingStage.Init(&mContext, mScene.get());
    auto rtImage = mRaytraycingStage.GetColorAttachmentByName(hsk::RaytracingStage::RaytracingRenderTargetName);

    UpdateOutputs();

    mImguiStage.Init(&mContext, mOutputs[mCurrentOutput]);
    PrepareImguiWindow();

    // �nit copy stage
    mImageToSwapchainStage.Init(&mContext, mOutputs[mCurrentOutput], hsk::ImageToSwapchainStage::PostCopy{.AccessFlags = (VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT), .ImageLayout = (VkImageLayout::VK_IMAGE_LAYOUT_GENERAL), .QueueFamilyIndex = (mContext.QueueGraphics)});
}

void ImportanceSamplingRtProject::RecordCommandBuffer(hsk::FrameRenderInfo &renderInfo)
{
    mScene->Update(renderInfo);
    mGbufferStage.RecordFrame(renderInfo);

#ifdef ENABLE_GBUFFER_BENCH
    if (mGbufferStage.GetBenchmark().GetLogs().size() > 0)
    {
        mDisplayedLog = mGbufferStage.GetBenchmark().GetLogs().back();
    }
#endif // ENABLE_GBUFFER_BENCH

    mRaytraycingStage.RecordFrame(renderInfo);

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
    auto normalImage = mGbufferStage.GetColorAttachmentByName(hsk::GBufferStage::WorldspaceNormal);
    mRaytraycingStage.OnResized(size);
    auto rtImage = mRaytraycingStage.GetColorAttachmentByName(hsk::RaytracingStage::RaytracingRenderTargetName);

    UpdateOutputs();

    mImguiStage.OnResized(size, mOutputs[mCurrentOutput]);
    mImageToSwapchainStage.OnResized(size, mOutputs[mCurrentOutput]);
}

void lUpdateOutput(std::unordered_map<std::string_view, hsk::ManagedImage *> &map, hsk::RenderStage &stage, const std::string_view name)
{
    map[name] = stage.GetColorAttachmentByName(name);
}

void ImportanceSamplingRtProject::UpdateOutputs()
{
    mOutputs.clear();
    lUpdateOutput(mOutputs, mGbufferStage, hsk::GBufferStage::Albedo);
    lUpdateOutput(mOutputs, mGbufferStage, hsk::GBufferStage::WorldspacePosition);
    lUpdateOutput(mOutputs, mGbufferStage, hsk::GBufferStage::WorldspaceNormal);
    lUpdateOutput(mOutputs, mGbufferStage, hsk::GBufferStage::MotionVector);
    lUpdateOutput(mOutputs, mGbufferStage, hsk::GBufferStage::MaterialIndex);
    lUpdateOutput(mOutputs, mGbufferStage, hsk::GBufferStage::MeshInstanceIndex);
    lUpdateOutput(mOutputs, mRaytraycingStage, hsk::RaytracingStage::RaytracingRenderTargetName);

    if (mCurrentOutput.size() == 0 || !mOutputs.contains(mCurrentOutput))
    {
        if (mOutputs.size() == 0)
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
    vkDeviceWaitIdle(mContext.Device);
    auto output = mOutputs[mCurrentOutput];
    mImguiStage.SetTargetImage(output);
    mImageToSwapchainStage.SetTargetImage(output);
}