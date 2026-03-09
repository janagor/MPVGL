#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stop_token>

#include <glslang/Public/ShaderLang.h>
#include <tl/expected.hpp>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Shader/ShaderCompiler.hpp"

namespace mpvgl {

class ShaderWatcher {
   public:
    ShaderWatcher(std::string const &shaderDir, std::string const &outputDir)
        : shaderDir(shaderDir), outputDir(outputDir) {
        std::filesystem::create_directories(outputDir);
    }

    ShaderWatcher(const ShaderWatcher &other) noexcept = delete;
    ShaderWatcher &operator=(const ShaderWatcher &other) noexcept = delete;
    ShaderWatcher(ShaderWatcher &&other) noexcept = delete;
    ShaderWatcher &operator=(ShaderWatcher &&other) noexcept = delete;

   public:
    inline void run(std::stop_token stopToken) {
        while (!stopToken.stop_requested()) {
            scanAndCompile();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        };
    }

    inline void compileAll() {
        for (const auto &entry :
             std::filesystem::directory_iterator(shaderDir)) {
            if (!isShaderFile(entry.path())) continue;

            if (auto result = compile(entry.path()); !result.has_value()) {
                std::cerr << "[ShaderWatcher] Initial Compilation Error: "
                          << result.error().message << "\n";
            }
        }
    }

    bool consumeChange() { return m_shadersChanged.exchange(false); }

   private:
    std::string shaderDir;
    std::string outputDir;
    std::map<std::string, std::filesystem::file_time_type> timestamps;
    std::atomic<bool> m_shadersChanged{false};

   private:
    inline bool isShaderFile(std::filesystem::path const &path) {
        return path.extension() == ".vert" || path.extension() == ".frag";
    }

    inline std::string outputPath(std::filesystem::path const &input) {
        return outputDir + "/" + input.filename().string() + ".spv";
    }

    inline tl::expected<void, Error> compile(
        std::filesystem::path const &shaderPath) {
        std::cout << "[ShaderWatcher] Compiling: " << shaderPath.filename()
                  << std::endl;

        std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
        if (!file) {
            return tl::unexpected(
                Error{EngineError::FileNotFound,
                      "Failed to open a file: " + shaderPath.string()});
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string result(size, '\0');
        if (!file.read(&result[0], size)) {
            return tl::unexpected(
                Error{EngineError::ShaderError,
                      "Cannot read file: " + shaderPath.string()});
        }

        ShaderCompiler compiler{};
        auto extention = shaderPath.extension();
        auto language = extention == ".vert" ? EShLanguage::EShLangVertex
                                             : EShLanguage::EShLangFragment;

        auto dataRes = compiler.compile(result, language);
        if (!dataRes.has_value()) {
            return tl::unexpected(dataRes.error());
        }

        auto &data = dataRes.value();
        std::ofstream ofile(outputPath(shaderPath), std::ios::binary);
        if (!ofile) {
            return tl::unexpected(Error{
                EngineError::FileNotFound,
                "Cannot open file for writing: " + outputPath(shaderPath)});
        }

        ofile.write(reinterpret_cast<const char *>(data.data()),
                    data.size() * sizeof(uint32_t));

        if (!ofile) {
            return tl::unexpected(Error{
                EngineError::ShaderError,
                "Failed to write data to file: " + outputPath(shaderPath)});
        }

        return {};
    }

    inline void scanAndCompile() {
        bool compiledSomething = false;
        for (const auto &entry :
             std::filesystem::directory_iterator(shaderDir)) {
            if (!isShaderFile(entry.path())) continue;

            std::string pathStr = entry.path().string();
            auto currentTime = std::filesystem::last_write_time(entry);

            if (timestamps.find(pathStr) == timestamps.end() ||
                timestamps[pathStr] != currentTime) {
                timestamps[pathStr] = currentTime;

                if (auto res = compile(entry.path()); res.has_value()) {
                    compiledSomething = true;
                } else {
                    std::cerr << "[ShaderWatcher] Hot-Reload Error: "
                              << res.error().message << "\n";
                }
            }
        }

        if (compiledSomething) {
            m_shadersChanged = true;
        }
    }
};

}  // namespace mpvgl
