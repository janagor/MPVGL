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
    [[nodiscard]] char const* name() const noexcept override {
        return "mpvgl::Engine";
    }

    [[nodiscard]] constexpr std::string message(int error) const override {
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

[[nodiscard]] inline std::error_category const& EngineErrorCategory() {
    static EngineErrorCategoryImpl const instance;
    return instance;
}

[[nodiscard]] inline std::error_code make_error_code(EngineError error) {
    return {static_cast<int>(error), EngineErrorCategory()};
}

}  // namespace mpvgl
