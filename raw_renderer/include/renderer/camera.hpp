#pragma once

#include "core/defines.hpp"
#include "events/event_handler.hpp"
#include "events/core_events.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Raw::GFX
{
    class Camera
    {
    public:
        Camera() {}
        ~Camera() {}

        void Init(f32 zNear, f32 zFar, f32 fov, f32 aspectRatio, const glm::vec3& pos,
             const glm::vec3& target, const glm::vec3& up);
        void Update(f32 dt);

        RAW_INLINE glm::mat4 GetDefaultViewMatrix() const { return m_DefaultView; }
        RAW_INLINE glm::mat4 GetViewMatrix() const 
        {
            const glm::mat4 translation = glm::translate(glm::mat4(1.f), -m_Position);
            const glm::mat4 rotation = glm::mat4_cast(m_Orientation); 
            return rotation * translation; 
        }
        RAW_INLINE glm::mat4 GetProjectionMatrix() const { return m_Proj; }
        RAW_INLINE glm::vec3 GetPosition() const { return m_Position; }

        RAW_INLINE void SetPositon(const glm::vec3& pos) { m_Position = pos; }
        RAW_INLINE void SetUpVector(const glm::vec3& up)
        {
            const glm::mat4 view = GetViewMatrix();
            const glm::vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
            m_Orientation = glm::lookAt(m_Position, m_Position + dir, up);
        }

        RAW_INLINE void RotateYaw(f32 yaw)
        {
            const glm::quat yawRot(glm::vec3(0, yaw, 0));
            m_Orientation = yawRot * m_Orientation;
        }

        RAW_INLINE void RotatePitch(f32 pitch)
        {
            const glm::quat pitchRot(glm::vec3(pitch, 0, 0));
            m_Orientation = pitchRot * m_Orientation;
        }

    public:
        f32 Acceleration{ 50.f };
        f32 MaxSpeed{ 100.f };
        f32 Damping{ 0.2f };
        f32 MouseSpeed{ 1.f };
        f32 MouseSensitivity{ 0.001f };
        f32 RotationSpeed{ 1.f };
        bool MoveForward{ false };
        bool MoveBack{ false };
        bool MoveRight{ false };
        bool MoveLeft{ false };
        bool MoveUp{ false };
        bool MoveDown{ false };

    private:
        glm::mat4 m_DefaultView{ 1.f };
        glm::mat4 m_Proj{ 1.f };
        glm::vec3 m_Position{ 1.f };
        glm::vec2 m_MousePos{ 0.f };
        glm::vec2 m_PrevMousePos{ 0.f };
        glm::quat m_Orientation{ glm::vec3(0.f) };
        glm::vec3 m_Rotation{ 0.f };
        glm::vec3 m_MoveSpeed{ 0.f };

        f32 m_Fov{ 0.f };
        f32 m_AspectRatio{ 0.f };
        f32 m_zNear{ 0.f };
        f32 m_zFar{ 0.f };

    private:
        bool OnWindowResize(const WindowResizeEvent& e);
        EventHandler<WindowResizeEvent> m_ResizeHandler;

    };
}