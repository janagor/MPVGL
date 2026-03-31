#pragma once

#include <cstddef>
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
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

struct GlslShaderIncluder : public glslang::TShader::Includer {
    auto includeSystem(char const * /*headerName*/,
                       char const * /*includerName*/, size_t /*inclusionDepth*/)
        -> IncludeResult * override {
        return nullptr;
    };

    auto includeLocal(char const * /*headerName*/,
                      char const * /*includerName*/, size_t /*inclusionDepth*/)
        -> IncludeResult * override {
        return nullptr;
    };

    void releaseInclude(IncludeResult * /*includeResult*/) override {};

   private:
    static auto getFailResult() -> IncludeResult * {
        static std::string const sEmpty;
        static IncludeResult sFailResult(sEmpty, "Header does not exist!", 0,
                                         nullptr);
        return &sFailResult;
    }

    std::map<std::string, IncludeResult *> mIncludes;
    std::map<std::string, std::vector<char>> mSources;
};

struct ShaderCompiler {
   public:
    ShaderCompiler() noexcept = default;

    ShaderCompiler(ShaderCompiler const &) = delete;
    auto operator=(ShaderCompiler const &) -> ShaderCompiler & = delete;
    ShaderCompiler(ShaderCompiler &&other) noexcept = delete;
    auto operator=(ShaderCompiler &&other) noexcept
        -> ShaderCompiler & = delete;
    ~ShaderCompiler() = default;

    static auto compile(std::string const &source, EShLanguage lang)
        -> tl::expected<std::vector<u32>, Error<EngineError>> {
        glslang::TShader shader{lang};

        auto sources = std::array<char const *, 1>{{source.c_str()}};
        shader.setStrings(sources.data(), 1);

        auto const targetApiVersion =
            glslang::EShTargetClientVersion{glslang::EShTargetVulkan_1_0};
        shader.setEnvClient(glslang::EShClientVulkan, targetApiVersion);

        auto const spirvVersion =
            glslang::EShTargetLanguageVersion{glslang::EShTargetSpv_1_0};
        shader.setEnvTarget(glslang::EshTargetSpv, spirvVersion);

        shader.setEntryPoint("main");

        auto *const resources = GetResources();
        resources->maxDrawBuffers = 1;
        auto const defaultVersion = 450;
        auto const defaultProfile = ENoProfile;
        auto const forceDefaultVersionAndProfile = false;
        auto const forwardCompatible = false;
        auto const messageFlags =
            (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
        GlslShaderIncluder includer{};

        std::string preprocessedStr;
        if (!shader.preprocess(resources, defaultVersion, defaultProfile,
                               forceDefaultVersionAndProfile, forwardCompatible,
                               messageFlags, &preprocessedStr, includer)) {
            return tl::unexpected{
                Error{EngineError::ShaderError,
                      std::string("Shader Preprocess failed: ") +
                          shader.getInfoLog()}};
        }

        auto preprocessedSources =
            std::array<char const *, 1>{{preprocessedStr.c_str()}};
        shader.setStrings(preprocessedSources.data(), 1);

        if (!shader.parse(resources, defaultVersion, defaultProfile, false,
                          forwardCompatible, messageFlags, includer)) {
            return tl::unexpected{Error{
                EngineError::ShaderError,
                std::string("Shader Parse failed:\n") + shader.getInfoLog()}};
        }

        glslang::TProgram program{};
        program.addShader(&shader);
        if (!program.link(messageFlags)) {
            return tl::unexpected{Error{
                EngineError::ShaderError,
                std::string("Shader Link failed: ") + program.getInfoLog()}};
        }

        glslang::TIntermediate const &intermediateRef =
            *(program.getIntermediate(lang));
        std::vector<u32> spirv;
        glslang::SpvOptions options{};
        options.validate = true;
        glslang::GlslangToSpv(intermediateRef, spirv, &options);

        return spirv;
    };
};

}  // namespace mpvgl
