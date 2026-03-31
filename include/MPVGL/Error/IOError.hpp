#pragma once

#include <string>
#include <system_error>
#include <type_traits>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

enum class IOError : u8 {
    Unknown = 1,
    FileNotFound,
    EndOfFile,
};

}

namespace std {

template <>
struct is_error_code_enum<mpvgl::IOError> : true_type {};

}  // namespace std

namespace mpvgl {

class IOErrorCategoryImpl : public std::error_category {
   public:
    [[nodiscard]] char const* name() const noexcept override {
        return "mpvgl::IO";
    }

    [[nodiscard]] constexpr std::string message(int error) const override {
        switch (static_cast<IOError>(error)) {
            case IOError::FileNotFound:
                return "File not found";
            case IOError::EndOfFile:
                return "End of file reached";
            default:
                return "Unknown IO error";
        }
    }
};

[[nodiscard]] inline std::error_category const& IOErrorCategory() {
    static IOErrorCategoryImpl const instance{};
    return instance;
}

[[nodiscard]] inline std::error_code make_error_code(IOError error) {
    return {static_cast<int>(error), IOErrorCategory()};
}

}  // namespace mpvgl
