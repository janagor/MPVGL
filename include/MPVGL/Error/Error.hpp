#pragma once

#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace mpvgl {

template <typename ErrorCategory>
class Error {
   public:
    Error(std::error_code errorCode, std::string message = "")
        : m_code(errorCode),
          m_message(message.empty() ? errorCode.message()
                                    : std::move(message)) {}

    Error(ErrorCategory err, std::string msg = "")
        : m_code(err),
          m_message(msg.empty() ? m_code.message() : std::move(msg)) {}

    [[nodiscard]] auto code() const noexcept -> std::error_code {
        return m_code;
    }
    [[nodiscard]] auto message() const noexcept -> std::string const& {
        return m_message;
    }

   private:
    std::error_code m_code;
    std::string m_message;
};

}  // namespace mpvgl
