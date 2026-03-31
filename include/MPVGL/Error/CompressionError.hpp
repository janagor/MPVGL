/**
 *  Your Project
 *
 *  Copyright (C) 2026 Jan Aleksander Górski
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would
 *     be appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source
 *     distribution.
 */
#pragma once

#include <string>
#include <system_error>
#include <type_traits>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

enum class CompressionError : u8 {
    Unknown = 1,
    InvalidData,
    TreeOverflow,
    UnsupportedFormat
};

}

namespace std {

template <>
struct is_error_code_enum<mpvgl::CompressionError> : true_type {};

}  // namespace std

namespace mpvgl {

class CompressionErrorCategoryImpl : public std::error_category {
   public:
    [[nodiscard]] char const* name() const noexcept override {
        return "mpvgl::Compression";
    }

    [[nodiscard]] std::string message(int error) const override {
        switch (static_cast<CompressionError>(error)) {
            case CompressionError::InvalidData:
                return "Invalid compression data or corrupted stream";
            case CompressionError::TreeOverflow:
                return "Maximum number of nodes in compression tree exceeded";
            case CompressionError::UnsupportedFormat:
                return "Unsupported compression format or parameters";
            default:
                return "Unknown compression error";
        }
    }
};

[[nodiscard]] inline std::error_category const& CompressionErrorCategory() {
    static CompressionErrorCategoryImpl instance;
    return instance;
}

[[nodiscard]] inline std::error_code make_error_code(CompressionError error) {
    return {static_cast<int>(error), CompressionErrorCategory()};
}

}  // namespace mpvgl
