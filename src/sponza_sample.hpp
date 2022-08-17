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
#include "stages/hsk_imagetoswapchain.hpp"
#include "stages/hsk_flipimage.hpp"
#include "stages/hsk_raytracingstage.hpp"

class ImportanceSamplingRtProject : public hsk::DefaultAppBase
{
public:
	ImportanceSamplingRtProject() = default;
	~ImportanceSamplingRtProject() = default;

protected:
	virtual void Init() override;
	virtual void OnEvent(const hsk::Event* event) override;
	virtual void Update(float delta) override;

	virtual void RecordCommandBuffer(hsk::FrameRenderInfo& renderInfo) override;
	virtual void OnResized(VkExtent2D size) override;
	virtual void Destroy() override;

	void PrepareImguiWindow();

	std::unique_ptr<hsk::Scene> mScene;

	void loadScene();

	/// @brief generates a GBuffer (Albedo, Positions, Normal, Motion Vectors, Mesh Instance Id as output images)
	hsk::GBufferStage mGbufferStage;
	/// @brief Renders immediate mode GUI
	hsk::ImguiStage mImguiStage;
	/// @brief Copies the intermediate rendertarget to the swapchain image
	hsk::ImageToSwapchainStage mImageToSwapchainStage;
	/// @brief Generates a raytraced image
	hsk::RaytracingStage mRaytraycingStage;

	void ConfigureStages();
};