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
#include <utility>

#include <tl/expected.hpp>

#include "MPVGL/Core/WindowConfig.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::glfw {

class Window {
   public:
    Window() = default;
    ~Window() noexcept;

    Window(Window const&) = delete;
    auto operator=(Window const&) -> Window& = delete;

    Window(Window&& other) noexcept;
    auto operator=(Window&& other) noexcept -> Window&;

    static auto create(WindowConfig const& config)
        -> tl::expected<Window, Error<EngineError>>;

    void pollEvents() const noexcept;
    [[nodiscard]] auto shouldClose() const noexcept -> bool;
    [[nodiscard]] auto getSize() const noexcept -> std::pair<int, int>;

    [[nodiscard]] auto getNativeHandle() const noexcept -> void*;

   private:
    explicit Window(void* nativeHandle) noexcept;
    void cleanup() noexcept;

    void* m_nativeHandle{nullptr};
};

}  // namespace mpvgl::glfw
