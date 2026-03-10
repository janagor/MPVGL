#pragma once

#include <string>
#include <system_error>
#include <type_traits>

namespace mpvgl {

enum class IOError {
    Unknown = 1,
    FileNotFound,
};

}

namespace std {

template <>
struct is_error_code_enum<mpvgl::IOError> : true_type {};

}  // namespace std

namespace mpvgl {

class IOErrorCategory_impl : public std::error_category {
   public:
    [[nodiscard]] char const* name() const noexcept override {
        return "mpvgl::IO";
    }

    [[nodiscard]] constexpr std::string message(int ev) const override {
        switch (static_cast<IOError>(ev)) {
            case IOError::FileNotFound:
                return "File not found";
            default:
                return "Unknown IO error";
        }
    }
};

[[nodiscard]] inline const std::error_category& IOErrorCategory() {
    static IOErrorCategory_impl instance;
    return instance;
}

[[nodiscard]] inline std::error_code make_error_code(IOError e) {
    return {static_cast<int>(e), IOErrorCategory()};
}

}  // namespace mpvgl
