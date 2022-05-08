#pragma once

#include <hsk_exception.hpp>

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include <glTF/hsk_glTF.hpp>
#include <hsk_env.hpp>
#include <hsk_rtrpf.hpp>
#include <stdint.h>

class ImportanceSamplingRtProject : public hsk::DefaultAppBase
{
public:
    ImportanceSamplingRtProject() = default;
    ~ImportanceSamplingRtProject() = default;

protected:
    virtual void Init() override;
    virtual void OnEvent(std::shared_ptr<hsk::Event> event) override;

private:
    hsk::Scene mScene;
    void initVulkan()
    {
        loadScene();
    }

    void loadScene()
    {
        // std::string fullFileName = hsk::MakeRelativePath("models/minimal.gltf");
        std::string fullFileName = hsk::MakeRelativePath("../sponza_model/Main/NewSponza_Main_Blender_glTF.gltf");
        // std::string fullFileName = hsk::MakeRelativePath("sponza_model/glTF-Sample-Models/2.0/Sponza/glTF/Sponza.gltf");

        // propagate vk variables
        auto context = mScene.GetVkContext();
        context->Allocator = mAllocator;
        context->Device = mDevice;
        context->PhysicalDevice = mPhysicalDevice;
        context->TransferCommandPool = mCommandPoolDefault;
        context->TransferQueue = mDefaultQueue.Queue;

        // mScene.LoadFromFile(fullFileName);
    }
};