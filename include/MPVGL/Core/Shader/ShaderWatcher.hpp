#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stop_token>

#include <glslang/Public/ShaderLang.h>
#include <tl/expected.hpp>

#include "MPVGL/Core/Shader/ShaderCompiler.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/IO/ResourceBuffer.hpp"

namespace mpvgl {

class ShaderWatcher {
   public:
    ShaderWatcher(std::string const &shaderDir, std::string const &outputDir)
        : shaderDir(shaderDir), outputDir(outputDir) {
        std::filesystem::create_directories(outputDir);
    }

    ShaderWatcher(ShaderWatcher const &other) noexcept = delete;
    ShaderWatcher &operator=(ShaderWatcher const &other) noexcept = delete;
    ShaderWatcher(ShaderWatcher &&other) noexcept = delete;
    ShaderWatcher &operator=(ShaderWatcher &&other) noexcept = delete;

   public:
    inline void run(std::stop_token const &stopToken) {
        while (!stopToken.stop_requested()) {
            scanAndCompile();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        };
    }

    inline void compileAll() {
        for (auto const &entry :
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

    inline tl::expected<void, Error<EngineError>> compile(
        std::filesystem::path const &shaderPath) {
        std::cout << "[ShaderWatcher] Compiling: " << shaderPath.filename()
                  << '\n';

        return io::ResourceBuffer::load(shaderPath)
            .map_error([](auto const &e) {
                return Error<EngineError>{EngineError::FileNotFound, e.message};
            })
            .and_then([&](io::ResourceBuffer const &buffer)
                          -> tl::expected<void, Error<EngineError>> {
                auto view = buffer.view();

                std::string source(reinterpret_cast<char const *>(view.data()),
                                   view.size());

                ShaderCompiler compiler{};
                auto extension = shaderPath.extension();
                auto language = (extension == ".vert")
                                    ? EShLanguage::EShLangVertex
                                    : EShLanguage::EShLangFragment;

                return compiler.compile(source, language)
                    .and_then([&](std::vector<u32> const &data)
                                  -> tl::expected<void, Error<EngineError>> {
                        std::ofstream ofile(outputPath(shaderPath),
                                            std::ios::binary);
                        if (!ofile) {
                            return tl::unexpected{
                                Error{EngineError::FileNotFound,
                                      "Cannot open file for writing: " +
                                          outputPath(shaderPath)}};
                        }

                        ofile.write(reinterpret_cast<char const *>(data.data()),
                                    data.size() * sizeof(u32));

                        if (!ofile) {
                            return tl::unexpected{
                                Error{EngineError::ShaderError,
                                      "Failed to write data to file: " +
                                          outputPath(shaderPath)}};
                        }
                        return {};
                    });
            });
    }

    inline void scanAndCompile() {
        bool compiledSomething = false;
        for (auto const &entry :
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
