#pragma once
#include <stages/foray_raytracingstage.hpp>

class CustomRtStage : public foray::stages::RaytracingStage
{
public:
    virtual void Init(const foray::core::VkContext *context, foray::scene::Scene *scene, foray::core::ManagedImage *envmap, foray::core::ManagedImage *noiseSource);
    virtual void CreateRaytraycingPipeline() override;
    virtual void OnShadersRecompiled(foray::base::ShaderCompiler *shaderCompiler) override;

    virtual void Destroy() override;
    virtual void DestroyShaders() override;

    struct RtStageShader
    {
        std::string Path = "";
        foray::core::ShaderModule Module;

        void Create(const foray::core::VkContext *context);
        void Destroy();
    };

protected:
    RtStageShader mRaygen{"shaders/raygen.rgen.spv"};
    RtStageShader mDefault_AnyHit{"shaders/ray-default/anyhit.rahit.spv"};
    RtStageShader mDefault_ClosestHit{"shaders/ray-default/closesthit.rchit.spv"};
    RtStageShader mDefault_Miss{"shaders/ray-default/miss.rmiss.spv"};
};
