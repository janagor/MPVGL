#pragma once

#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace mpvgl {

template <typename ErrorCategory>
class Error {
   public:
    Error(std::error_code ec, std::string msg = "")
        : m_code(ec), m_message(msg.empty() ? ec.message() : std::move(msg)) {}

    Error(ErrorCategory err, std::string msg = "")
        : m_code(err),
          m_message(msg.empty() ? m_code.message() : std::move(msg)) {}

    [[nodiscard]] std::error_code code() const noexcept { return m_code; }

    [[nodiscard]] std::string const& message() const noexcept {
        return m_message;
    }

   private:
    std::error_code m_code;
    std::string m_message;
};

}  // namespace mpvgl
