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
    [[nodiscard]] auto name() const noexcept -> char const* override {
        return "mpvgl::IO";
    }

    [[nodiscard]] constexpr auto message(int error) const
        -> std::string override {
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

[[nodiscard]] inline auto IOErrorCategory() -> std::error_category const& {
    static IOErrorCategoryImpl const instance{};
    return instance;
}

[[nodiscard]] inline auto make_error_code(IOError error) -> std::error_code {
    return {static_cast<int>(error), IOErrorCategory()};
}

}  // namespace mpvgl
