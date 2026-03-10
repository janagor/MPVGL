#pragma once

#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace mpvgl {

template <typename ErrorCategory>
struct Error {
    Error(std::error_code ec, std::string msg = "")
        : code(ec), message(msg.empty() ? ec.message() : std::move(msg)) {}

    Error(ErrorCategory err, std::string msg = "")
        : code(err), message(msg.empty() ? code.message() : std::move(msg)) {}

    std::error_code code;
    std::string message;
};

}  // namespace mpvgl
