#pragma once

#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace mpvgl {

enum class EngineError {
    VulkanInitFailed = 1,
    VulkanRuntimeError,
    ShaderError,
    FileNotFound,
    WindowError,
    Unknown
};

}  // namespace mpvgl

namespace std {

template <>
struct is_error_code_enum<mpvgl::EngineError> : true_type {};

}  // namespace std

namespace mpvgl {

class EngineErrorCategory_impl : public std::error_category {
   public:
    [[nodiscard]] char const* name() const noexcept override {
        return "mpvgl::Engine";
    }

    [[nodiscard]] constexpr std::string message(int ev) const override {
        switch (static_cast<EngineError>(ev)) {
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

[[nodiscard]] inline const std::error_category& EngineErrorCategory() {
    static EngineErrorCategory_impl instance;
    return instance;
}

[[nodiscard]] inline std::error_code make_error_code(EngineError e) {
    return {static_cast<int>(e), EngineErrorCategory()};
}

struct Error {
    Error(std::error_code ec, std::string msg = "")
        : code(ec), message(msg.empty() ? ec.message() : std::move(msg)) {}

    Error(EngineError err, std::string msg = "")
        : code(err), message(msg.empty() ? code.message() : std::move(msg)) {}

    std::error_code code;
    std::string message;
};

}  // namespace mpvgl
