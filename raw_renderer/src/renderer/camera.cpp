#include "renderer/camera.hpp"
#include "core/input.hpp"
#include "core/keycodes.hpp"
#include "core/mouse.hpp"
#include "events/event_manager.hpp"
#include <algorithm>

namespace Raw::GFX
{
    void Camera::Init(f32 zNear, f32 zFar, f32 fov, f32 aspectRatio, const glm::vec3& pos,
             const glm::vec3& target, const glm::vec3& up)
    {
        m_Position = pos;
        m_Orientation = glm::lookAt(pos, target, up);
        m_Fov = fov;
        m_AspectRatio = aspectRatio;
        m_zNear = zNear;
        m_zFar = zFar;
        m_MousePos = glm::vec2(Input::Get()->GetMousePosition().first, Input::Get()->GetMousePosition().second);
        m_PrevMousePos = m_MousePos;

        m_Proj = glm::perspective(fov, aspectRatio, zNear, zFar);

        const glm::mat4 translation = glm::translate(glm::mat4(1.f), -m_Position);
        const glm::mat4 rotation = glm::mat4_cast(m_Orientation);     

        m_DefaultView = rotation * translation;

        m_ResizeHandler = BIND_EVENT_FN(Camera::OnWindowResize);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ResizeHandler, WindowResizeEvent), WindowResizeEvent::GetStaticEventType());
    }

    void Camera::Update(f32 dt)
    {
        if(Input::Get()->IsMouseButtonPressed(RAW_BUTTON_RIGHT))
        {
            auto curPos = Input::Get()->GetMousePosition();
            auto prevPos = Input::Get()->GetPreviousMousePosition();
            u32 diffX = curPos.first - prevPos.first;
            u32 diffY = curPos.second - prevPos.second;
            if(diffX != 0 || diffY != 0)
            {
                m_MousePos = glm::vec2(curPos.first, curPos.second);
                if(m_PrevMousePos != m_MousePos)
                {
                    m_PrevMousePos = glm::vec2(prevPos.first, prevPos.second);
                    
                    const glm::vec2 delta = (m_MousePos - m_PrevMousePos) * MouseSensitivity;
                    m_Rotation.x += delta.x;
                    m_Rotation.y += delta.y;
                    const glm::quat deltaQuatX = glm::quat(glm::vec3(m_Rotation.y, 0.0f, 0.0f));
                    const glm::quat deltaQuatY = glm::quat(glm::vec3(0.0f, -m_Rotation.x, 0.0f));
                    const glm::quat deltaQuat = glm::quat(deltaQuatX * deltaQuatY);
                    m_Orientation = glm::normalize(deltaQuat);
                    m_PrevMousePos = m_MousePos;
                }
            }
        }
        
		const glm::mat4 view = GetViewMatrix();
		const glm::vec3 forward = -glm::vec3(view[0][2], view[1][2], view[2][2]);
		const glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
		const glm::vec3 up = glm::cross(right, forward);
		SetUpVector(up);

        MoveForward = Input::Get()->IsKeyPressed(RAW_KEY_W);
        MoveBack = Input::Get()->IsKeyPressed(RAW_KEY_S);
        MoveLeft = Input::Get()->IsKeyPressed(RAW_KEY_A);
        MoveRight = Input::Get()->IsKeyPressed(RAW_KEY_D);
        MoveUp = Input::Get()->IsKeyPressed(RAW_KEY_Q);
        MoveDown = Input::Get()->IsKeyPressed(RAW_KEY_E);
        
        glm::vec3 accel(0);
		if (MoveForward)         accel += forward;
		if (MoveBack)            accel -= forward;
		if (MoveRight)           accel -= right;
		if (MoveLeft)            accel += right;
		if (MoveUp)              accel += up;
		if (MoveDown)            accel -= up;
        
        if(accel == glm::vec3(0))
        {
            m_MoveSpeed -= m_MoveSpeed * std::min( 1.0f / Damping * dt, 1.0f );
        }
		else
		{
            m_MoveSpeed += accel * Acceleration * dt;
			if(glm::length(m_MoveSpeed) > MaxSpeed)
			{
                m_MoveSpeed = glm::normalize(m_MoveSpeed) * MaxSpeed;
			}
		}
        
		m_Position += m_MoveSpeed * dt;
    }

    bool Camera::OnWindowResize(const WindowResizeEvent& e)
    {
        f32 aspectRatio = (f32)e.GetWidth() / (f32)e.GetHeight();
        if(aspectRatio != m_AspectRatio)
        {
            m_AspectRatio = aspectRatio;
            m_Proj = glm::perspective(m_Fov, m_AspectRatio, m_zNear, m_zFar);
        }
        return false;
    }
}