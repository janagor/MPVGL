#pragma once

#include <filesystem>

namespace mpvgl {

enum struct ShaderType { Vertex, Fragment };
enum struct ShaderStage { SourceCode, Binary };

template <ShaderType T, ShaderStage S> struct Shader {
public:
  constexpr Shader() noexcept = delete;
  Shader(std::filesystem::path const &path) : m_path(path) {};
  ~Shader() = default;

private:
  std::filesystem::path m_path;
};

} // namespace mpvgl
