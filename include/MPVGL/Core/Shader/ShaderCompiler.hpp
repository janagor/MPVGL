#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <glslang/MachineIndependent/Versions.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <tl/expected.hpp>

#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl {

struct GlslShaderIncluder : public glslang::TShader::Includer {
    virtual IncludeResult *includeSystem(const char *headerName,
                                         const char *includerName,
                                         size_t inclusionDepth) override {
        return nullptr;
    };

    virtual IncludeResult *includeLocal(const char *headerName,
                                        const char *includerName,
                                        size_t inclusionDepth) override {
        return nullptr;
    };

    virtual void releaseInclude(IncludeResult *) override {};

   private:
    static inline const std::string sEmpty = "";
    static inline IncludeResult smFailResult =
        IncludeResult(sEmpty, "Header does not exist!", 0, nullptr);

    std::map<std::string, IncludeResult *> mIncludes;
    std::map<std::string, std::vector<char>> mSources;
};

struct ShaderCompiler {
   public:
    ShaderCompiler() noexcept {};

    ShaderCompiler(const ShaderCompiler &) = delete;
    ShaderCompiler &operator=(const ShaderCompiler &) = delete;
    ShaderCompiler(ShaderCompiler &&other) noexcept = delete;
    ShaderCompiler &operator=(ShaderCompiler &&other) noexcept = delete;

   public:
    // ZMIANA: Zwracamy tl::expected zamiast samego wektora!
    tl::expected<std::vector<uint32_t>, Error<EngineError>> compile(
        const std::string &source, EShLanguage lang) {
        glslang::TShader shader(lang);
        const char *sources[1] = {source.data()};
        shader.setStrings(sources, 1);
        glslang::EShTargetClientVersion targetApiVersion =
            glslang::EShTargetVulkan_1_0;
        shader.setEnvClient(glslang::EShClientVulkan, targetApiVersion);

        glslang::EShTargetLanguageVersion spirvVersion =
            glslang::EShTargetSpv_1_0;
        shader.setEnvTarget(glslang::EshTargetSpv, spirvVersion);

        shader.setEntryPoint("main");

        const auto resources = GetResources();
        resources->maxDrawBuffers = 1;
        const auto defaultVersion = 450;
        const auto defaultProfile = ENoProfile;
        const auto forceDefaultVersionAndProfile = false;
        const auto forwardCompatible = false;
        const auto messageFlags =
            (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
        GlslShaderIncluder includer{};

        std::string preprocessedStr;
        if (!shader.preprocess(resources, defaultVersion, defaultProfile,
                               forceDefaultVersionAndProfile, forwardCompatible,
                               messageFlags, &preprocessedStr, includer)) {
            return tl::unexpected(
                Error{EngineError::ShaderError,
                      std::string("Shader Preprocess failed: ") +
                          shader.getInfoLog()});
        }

        const char *preprocessedSources[1] = {preprocessedStr.c_str()};
        shader.setStrings(preprocessedSources, 1);

        if (!shader.parse(resources, defaultVersion, defaultProfile, false,
                          forwardCompatible, messageFlags, includer)) {
            return tl::unexpected(Error{
                EngineError::ShaderError,
                std::string("Shader Parse failed:\n") + shader.getInfoLog()});
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messageFlags)) {
            return tl::unexpected(Error{
                EngineError::ShaderError,
                std::string("Shader Link failed: ") + program.getInfoLog()});
        }

        glslang::TIntermediate &intermediateRef =
            *(program.getIntermediate(lang));
        std::vector<uint32_t> spirv;
        glslang::SpvOptions options{};
        options.validate = true;
        glslang::GlslangToSpv(intermediateRef, spirv, &options);

        return spirv;
    };
};

}  // namespace mpvgl
