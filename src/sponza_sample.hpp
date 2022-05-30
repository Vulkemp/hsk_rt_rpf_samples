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
#include "stages/hsk_gbuffer.hpp"

class ImportanceSamplingRtProject : public hsk::DefaultAppBase
{
public:
    ImportanceSamplingRtProject() = default;
    ~ImportanceSamplingRtProject() = default;

protected:
    virtual void Init() override;
    virtual void OnEvent(std::shared_ptr<hsk::Event> event) override;

    virtual void Cleanup() override;

    hsk::Scene mScene;

    void loadScene();

    hsk::GBufferStage mGbufferStage;

    void ConfigureStages();
};