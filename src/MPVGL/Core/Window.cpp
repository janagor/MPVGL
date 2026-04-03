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
#include <iostream>
#include <utility>

#include <tl/expected.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Window.hpp"
#include "MPVGL/Core/WindowConfig.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::glfw {

Window::Window(void* nativeHandle) noexcept : m_nativeHandle(nativeHandle) {}

Window::Window(Window&& other) noexcept
    : m_nativeHandle(std::exchange(other.m_nativeHandle, nullptr)) {}

auto Window::operator=(Window&& other) noexcept -> Window& {
    if (this != &other) {
        cleanup();
        m_nativeHandle = std::exchange(other.m_nativeHandle, nullptr);
    }
    return *this;
}

Window::~Window() noexcept { cleanup(); }

void Window::cleanup() noexcept {
    if (m_nativeHandle != nullptr) {
        glfwDestroyWindow(static_cast<GLFWwindow*>(m_nativeHandle));
        glfwTerminate();
        m_nativeHandle = nullptr;
    }
}

auto Window::create(WindowConfig const& config)
    -> tl::expected<Window, Error<EngineError>> {
    glfwSetErrorCallback([](int error, char const* description) -> void {
        std::cerr << "GLFW Error (" << error << "): " << description << '\n';
    });

    if (glfwInit() == GLFW_FALSE) {
        return tl::unexpected{
            Error{EngineError::WindowError, "Failed to initialize GLFW"}};
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!config.resizable) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    GLFWwindow* window = glfwCreateWindow(
        config.width, config.height, config.title.c_str(), nullptr, nullptr);

    if (window == nullptr) {
        glfwTerminate();
        return tl::unexpected{
            Error{EngineError::WindowError, "Failed to create GLFW window!"}};
    }

    return Window{static_cast<void*>(window)};
}

// INFO: in the future we want to support other APIs so this could (?) be useful
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Window::pollEvents() const noexcept { glfwPollEvents(); }

auto Window::shouldClose() const noexcept -> bool {
    if (nullptr == m_nativeHandle) {
        return true;
    }
    return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_nativeHandle)) ==
           GLFW_TRUE;
}

auto Window::getSize() const noexcept -> std::pair<int, int> {
    if (nullptr == m_nativeHandle) {
        return {0, 0};
    }
    int width = 0;
    int height = 0;
    glfwGetWindowSize(static_cast<GLFWwindow*>(m_nativeHandle), &width,
                      &height);
    return {width, height};
}

auto Window::getNativeHandle() const noexcept -> void* {
    return m_nativeHandle;
}

}  // namespace mpvgl::glfw
