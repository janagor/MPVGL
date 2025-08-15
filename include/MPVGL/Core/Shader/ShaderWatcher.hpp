#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stop_token>
#include <string>
#include <thread>

#include "MPVGL/Core/Shader/ShaderCompiler.hpp"
#include <glslang/Public/ShaderLang.h>

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
      std::this_thread::sleep_for(std::chrono::seconds(1));
    };
  }

  inline void compileAll() {
    for (const auto &entry : std::filesystem::directory_iterator(shaderDir)) {
      if (!isShaderFile(entry.path()))
        continue;
      else
        compile(entry.path());
    }
  }

private:
  std::string shaderDir;
  std::string outputDir;
  std::map<std::string, std::filesystem::file_time_type> timestamps;

private:
  inline bool isShaderFile(std::filesystem::path const &path) {
    return path.extension() == ".vert" || path.extension() == ".frag";
  }
  inline std::string outputPath(std::filesystem::path const &input) {
    return outputDir + "/" + input.filename().string() + ".spv";
  }

  inline void compile2(const std::filesystem::path &shaderPath) {
    std::string command = "glslangValidator -V " + shaderPath.string() +
                          " -o " + outputPath(shaderPath);
    std::cout << "[ShaderWatcher] Compiling: " << shaderPath.filename() << "\n";

    int result = std::system(command.c_str());
    if (result != 0) {
      std::cerr << "[ShaderWatcher] Compilation failed: " << shaderPath << "\n";
    }
  }

  inline void compile(const std::filesystem::path &shaderPath) {
    std::cout << shaderPath << std::endl;
    std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
    if (!file)
      throw std::runtime_error("Cannot open file");

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string result(size, '\0');
    if (!file.read(&result[0], size)) {
      throw std::runtime_error("Cannot read file");
    }
    ShaderCompiler compiler{};
    auto extention = shaderPath.extension();
    auto language = extention == ".vert" ? EShLanguage::EShLangVertex
                                         : EShLanguage::EShLangFragment;
    auto data = compiler.compile(result, language);

    std::ofstream ofile(outputPath(shaderPath), std::ios::binary);
    if (!file) {
      throw std::runtime_error("Cannot open file for writing: " +
                               outputPath(shaderPath));
    }

    ofile.write(reinterpret_cast<const char *>(data.data()),
                data.size() * sizeof(uint32_t));

    if (!ofile) {
      throw std::runtime_error("Failed to write data to file: " +
                               outputPath(shaderPath));
    }
  }

  inline void scanAndCompile() {
    for (const auto &entry : std::filesystem::directory_iterator(shaderDir)) {
      if (!isShaderFile(entry.path()))
        continue;

      std::string pathStr = entry.path().string();
      auto currentTime = std::filesystem::last_write_time(entry);

      if (timestamps.find(pathStr) == timestamps.end() ||
          timestamps[pathStr] != currentTime) {
        timestamps[pathStr] = currentTime;
        compile(entry.path());
      }
    }
  }
};

} // namespace mpvgl
