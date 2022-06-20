#pragma once

#include <hsk_exception.hpp>

#include <hsk_glm.hpp>
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

#include <scenegraph/hsk_scenegraph.hpp>
#include <hsk_env.hpp>
#include <hsk_rtrpf.hpp>
#include <stdint.h>
#include "stages/hsk_gbuffer.hpp"
#include "stages/hsk_imguistage.hpp"

class ImportanceSamplingRtProject : public hsk::DefaultAppBase
{
public:
    ImportanceSamplingRtProject() = default;
    ~ImportanceSamplingRtProject() = default;

protected:
    virtual void Init() override;
    virtual void OnEvent(std::shared_ptr<hsk::Event> event) override;
    virtual void Update(float delta);

    virtual void RecordCommandBuffer(hsk::FrameRenderInfo& renderInfo) override;
    virtual void OnResized(VkExtent2D size) override;
    virtual void Cleanup() override;
    
    void RecordImguiWindow(hsk::FrameRenderInfo& renderInfo);

    std::unique_ptr<hsk::Scene> mScene;

    void loadScene();

    hsk::GBufferStage mGbufferStage;
    hsk::ImguiStage mImguiStage;

    void ConfigureStages();
};