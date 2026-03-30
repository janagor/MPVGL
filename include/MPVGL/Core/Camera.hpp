#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
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

constexpr f32 SPEED = 2.5f;
constexpr f32 SENSITIVITY = 0.1f;
constexpr f32 ZOOM = 45.0f;

class Camera {
   public:
    [[nodiscard]] glm::vec3 const& position() const noexcept {
        return m_position;
    }
    [[nodiscard]] glm::quat const& orientation() const noexcept {
        return m_orientation;
    }
    [[nodiscard]] glm::vec3 const& front() const noexcept { return m_front; }
    [[nodiscard]] glm::vec3 const& up() const noexcept { return m_up; }
    [[nodiscard]] glm::vec3 const& right() const noexcept { return m_right; }

    [[nodiscard]] f32 movementSpeed() const noexcept { return m_movementSpeed; }
    [[nodiscard]] f32 mouseSensitivity() const noexcept {
        return m_mouseSensitivity;
    }
    [[nodiscard]] f32 zoom() const noexcept { return m_zoom; }

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f))
        : m_movementSpeed(SPEED),
          m_mouseSensitivity(SENSITIVITY),
          m_zoom(ZOOM) {
        m_position = position;
        auto const lookAt = glm::lookAt(position, glm::vec3(0.0f, 0.0f, 0.0f),
                                        glm::vec3(0.0f, 0.0f, 1.0f));
        m_orientation = glm::conjugate(glm::quat_cast(lookAt));
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        auto const rotate = glm::mat4_cast(glm::conjugate(m_orientation));
        auto const translate = glm::translate(glm::mat4(1.0f), -m_position);
        return rotate * translate;
    }

    void processKeyboard(CameraMovement direction, f32 deltaTime) {
        f32 const velocity = m_movementSpeed * deltaTime;

        if (direction == CameraMovement::Forward)
            m_position += m_front * velocity;
        if (direction == CameraMovement::Backward)
            m_position -= m_front * velocity;
        if (direction == CameraMovement::Left) m_position -= m_right * velocity;
        if (direction == CameraMovement::Right)
            m_position += m_right * velocity;
        if (direction == CameraMovement::Up) m_position += m_up * velocity;
        if (direction == CameraMovement::Down) m_position -= m_up * velocity;

        if (direction == CameraMovement::RollLeft) {
            auto const qRoll =
                glm::angleAxis(glm::radians(-50.0f * deltaTime), m_front);
            m_orientation = qRoll * m_orientation;
            m_orientation = glm::normalize(m_orientation);
            updateCameraVectors();
        }
        if (direction == CameraMovement::RollRight) {
            auto const qRoll =
                glm::angleAxis(glm::radians(50.0f * deltaTime), m_front);
            m_orientation = qRoll * m_orientation;
            m_orientation = glm::normalize(m_orientation);
            updateCameraVectors();
        }
    }

    void processMouseMovement(f32 xoffset, f32 yoffset) {
        auto const yawAmount = xoffset * m_mouseSensitivity;
        auto const pitchAmount = yoffset * m_mouseSensitivity;

        auto const qYaw = glm::angleAxis(glm::radians(-yawAmount), m_up);
        auto const qPitch = glm::angleAxis(glm::radians(-pitchAmount), m_right);

        m_orientation = qYaw * qPitch * m_orientation;
        m_orientation = glm::normalize(m_orientation);

        updateCameraVectors();
    }

    void processMouseScroll(f32 yoffset) {
        m_zoom -= yoffset;
        if (m_zoom < 1.0f) m_zoom = 1.0f;
        if (m_zoom > 45.0f) m_zoom = 45.0f;
    }

   private:
    void updateCameraVectors() {
        m_front = glm::normalize(m_orientation * glm::vec3(0.0f, 0.0f, -1.0f));
        m_right = glm::normalize(m_orientation * glm::vec3(1.0f, 0.0f, 0.0f));
        m_up = glm::normalize(m_orientation * glm::vec3(0.0f, 1.0f, 0.0f));
    }

   private:
    glm::vec3 m_position;
    glm::quat m_orientation;

    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;

    f32 m_movementSpeed;
    f32 m_mouseSensitivity;
    f32 m_zoom;
};

}  // namespace mpvgl
