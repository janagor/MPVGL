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
#include <glm/gtx/quaternion.hpp>
#include <glm/trigonometric.hpp>

namespace mpvgl {

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
    RollLeft,
    RollRight
};

constexpr float SPEED = 2.5f;
constexpr float SENSITIVITY = 0.1f;
constexpr float ZOOM = 45.0f;

class Camera {
   public:
    glm::vec3 Position;

    glm::quat Orientation;

    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f))
        : MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position = position;
        glm::mat4 lookAt = glm::lookAt(position, glm::vec3(0.0f, 0.0f, 0.0f),
                                       glm::vec3(0.0f, 0.0f, 1.0f));
        Orientation = glm::conjugate(glm::quat_cast(lookAt));
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        glm::mat4 rotate = glm::mat4_cast(glm::conjugate(Orientation));
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), -Position);
        return rotate * translate;
    }

    void processKeyboard(CameraMovement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;

        if (direction == CameraMovement::Forward) Position += Front * velocity;
        if (direction == CameraMovement::Backward) Position -= Front * velocity;
        if (direction == CameraMovement::Left) Position -= Right * velocity;
        if (direction == CameraMovement::Right) Position += Right * velocity;
        if (direction == CameraMovement::Up) Position += Up * velocity;
        if (direction == CameraMovement::Down) Position -= Up * velocity;

        if (direction == CameraMovement::RollLeft) {
            glm::quat qRoll =
                glm::angleAxis(glm::radians(-50.0f * deltaTime), Front);
            Orientation = qRoll * Orientation;
            Orientation = glm::normalize(Orientation);
            updateCameraVectors();
        }
        if (direction == CameraMovement::RollRight) {
            glm::quat qRoll =
                glm::angleAxis(glm::radians(50.0f * deltaTime), Front);
            Orientation = qRoll * Orientation;
            Orientation = glm::normalize(Orientation);
            updateCameraVectors();
        }
    }

    void processMouseMovement(float xoffset, float yoffset) {
        float yawAmount = xoffset * MouseSensitivity;
        float pitchAmount = yoffset * MouseSensitivity;

        glm::quat qYaw = glm::angleAxis(glm::radians(-yawAmount), Up);

        glm::quat qPitch = glm::angleAxis(glm::radians(-pitchAmount), Right);

        Orientation = qYaw * qPitch * Orientation;
        Orientation = glm::normalize(Orientation);

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

   private:
    void updateCameraVectors() {
        Front = glm::normalize(Orientation * glm::vec3(0.0f, 0.0f, -1.0f));
        Right = glm::normalize(Orientation * glm::vec3(1.0f, 0.0f, 0.0f));
        Up = glm::normalize(Orientation * glm::vec3(0.0f, 1.0f, 0.0f));
    }
};

}  // namespace mpvgl
