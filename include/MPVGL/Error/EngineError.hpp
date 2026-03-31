#pragma once

#include <string>
#include <system_error>
#include <type_traits>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

enum class EngineError : u8 {
    Unknown = 1,
    VulkanInitFailed,
    VulkanRuntimeError,
    ShaderError,
    FileNotFound,
    WindowError,
};

}

namespace std {

template <>
struct is_error_code_enum<mpvgl::EngineError> : true_type {};

}  // namespace std

namespace mpvgl {

class EngineErrorCategoryImpl : public std::error_category {
   public:
    [[nodiscard]] auto name() const noexcept -> char const* override {
        return "mpvgl::Engine";
    }

    [[nodiscard]] constexpr auto message(int error) const
        -> std::string override {
        switch (static_cast<EngineError>(error)) {
            case EngineError::VulkanInitFailed:
                return "Vulkan initialization failed";
            case EngineError::VulkanRuntimeError:
                return "Vulkan runtime error";
            case EngineError::ShaderError:
                return "Shader error";
            case EngineError::FileNotFound:
                return "File not found";
            case EngineError::WindowError:
                return "Window error";
            default:
                return "Unknown engine error";
        }
    }
};

[[nodiscard]] inline auto EngineErrorCategory() -> std::error_category const& {
    static EngineErrorCategoryImpl const instance;
    return instance;
}

[[nodiscard]] inline auto make_error_code(EngineError error)
    -> std::error_code {
    return {static_cast<int>(error), EngineErrorCategory()};
}

}  // namespace mpvgl
