#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include <algorithm>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

enum class CameraMovement : u8 {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
    RollLeft,
    RollRight
};

constexpr f64 SPEED = 2.5;
constexpr f64 SENSITIVITY = 0.1;
constexpr f64 DEFAULT_ZOOM = 45.0;
constexpr f64 MIN_ZOOM = 1.0;
constexpr f64 MAX_ZOOM = 45.0;

class Camera {
   public:
    [[nodiscard]] auto position() const noexcept -> glm::dvec3 const& {
        return m_position;
    }
    [[nodiscard]] auto orientation() const noexcept -> glm::dquat const& {
        return m_orientation;
    }
    [[nodiscard]] auto front() const noexcept -> glm::dvec3 const& {
        return m_front;
    }
    [[nodiscard]] auto up() const noexcept -> glm::dvec3 const& { return m_up; }
    [[nodiscard]] auto right() const noexcept -> glm::dvec3 const& {
        return m_right;
    }

    [[nodiscard]] auto movementSpeed() const noexcept -> f64 {
        return m_movementSpeed;
    }
    [[nodiscard]] auto mouseSensitivity() const noexcept -> f64 {
        return m_mouseSensitivity;
    }
    [[nodiscard]] auto zoom() const noexcept -> f64 { return m_zoom; }

    Camera(glm::dvec3 position = glm::dvec3(0.0, 0.0, 0.0))
        : m_position(position) {
        auto const lookAt = glm::lookAt(position, glm::dvec3(0.0, 0.0, 0.0),
                                        glm::dvec3(0.0, 0.0, 1.0));
        m_orientation = glm::conjugate(glm::quat_cast(lookAt));
        updateCameraVectors();
    }

    [[nodiscard]] auto getViewMatrix() const -> glm::mat4 {
        auto const rotate = glm::mat4_cast(glm::conjugate(m_orientation));
        auto const translate = glm::translate(glm::dmat4(1.0), -m_position);
        return rotate * translate;
    }

    void processKeyboard(CameraMovement direction, f64 deltaTime) {
        auto const velocity = m_movementSpeed * deltaTime;

        if (direction == CameraMovement::Forward) {
            m_position += m_front * velocity;
        }
        if (direction == CameraMovement::Backward) {
            m_position -= m_front * velocity;
        }
        if (direction == CameraMovement::Left) {
            m_position -= m_right * velocity;
        }
        if (direction == CameraMovement::Right) {
            m_position += m_right * velocity;
        }
        if (direction == CameraMovement::Up) {
            m_position += m_up * velocity;
        }
        if (direction == CameraMovement::Down) {
            m_position -= m_up * velocity;
        }

        if (direction == CameraMovement::RollLeft) {
            auto const qRoll =
                glm::angleAxis(glm::radians(-50.0F * deltaTime), m_front);
            m_orientation = qRoll * m_orientation;
            m_orientation = glm::normalize(m_orientation);
            updateCameraVectors();
        }
        if (direction == CameraMovement::RollRight) {
            auto const qRoll =
                glm::angleAxis(glm::radians(50.0F * deltaTime), m_front);
            m_orientation = qRoll * m_orientation;
            m_orientation = glm::normalize(m_orientation);
            updateCameraVectors();
        }
    }

    void processMouseMovement(f64 xoffset, f64 yoffset) {
        auto const yawAmount = xoffset * m_mouseSensitivity;
        auto const pitchAmount = yoffset * m_mouseSensitivity;

        auto const qYaw = glm::angleAxis(glm::radians(-yawAmount), m_up);
        auto const qPitch = glm::angleAxis(glm::radians(-pitchAmount), m_right);

        m_orientation = qYaw * qPitch * m_orientation;
        m_orientation = glm::normalize(m_orientation);

        updateCameraVectors();
    }

    void processMouseScroll(f64 yoffset) {
        m_zoom -= yoffset;
        m_zoom = std::clamp(m_zoom, MIN_ZOOM, MAX_ZOOM);
    }

   private:
    void updateCameraVectors() {
        m_front = glm::normalize(m_orientation * glm::dvec3(0.0, 0.0, -1.0));
        m_right = glm::normalize(m_orientation * glm::dvec3(1.0, 0.0, 0.0));
        m_up = glm::normalize(m_orientation * glm::dvec3(0.0, 1.0, 0.0));
    }

    glm::dvec3 m_position{};
    glm::dquat m_orientation{};

    glm::dvec3 m_front{};
    glm::dvec3 m_up{};
    glm::dvec3 m_right{};

    f64 m_movementSpeed{SPEED};
    f64 m_mouseSensitivity{SENSITIVITY};
    f64 m_zoom{DEFAULT_ZOOM};
};

}  // namespace mpvgl
