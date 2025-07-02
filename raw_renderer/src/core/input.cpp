#include "core/input.hpp"
#include "core/keycodes.hpp"
#include "core/mouse.hpp"
#include "events/event_manager.hpp"
#include "core/logger.hpp"
#include <unordered_map>

namespace Raw
{
    static std::unordered_map<u32,bool> g_KeyCodes;
    static bool g_MouseButtons[NUM_MOUSE_BUTTONS];
    static std::pair<u32,u32> g_MousePos = { 0, 0 };
    static std::pair<u32,u32> g_PrevMousePos = { 0, 0 };
    static Input s_Input;

    Input* Input::Get()
    {
        return &s_Input;
    }

    void Input::InitializeInput()
    {
        m_KeyPressedHandler = BIND_EVENT_FN(Input::OnKeyPressed);
        m_KeyReleasedHandler = BIND_EVENT_FN(Input::OnKeyReleased);
        m_ButtonPressedHandler = BIND_EVENT_FN(Input::OnMouseButtonPressed);
        m_ButtonReleasedHandler = BIND_EVENT_FN(Input::OnMouseButtonReleased);
        m_MouseMovedHandler = BIND_EVENT_FN(Input::OnMouseMoved);

        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_KeyPressedHandler, KeyPressedEvent), KeyPressedEvent::GetStaticEventType());
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_KeyReleasedHandler, KeyReleasedEvent), KeyReleasedEvent::GetStaticEventType());
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ButtonPressedHandler, MouseButtonPressedEvent), MouseButtonPressedEvent::GetStaticEventType());
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ButtonReleasedHandler, MouseButtonReleasedEvent), MouseButtonReleasedEvent::GetStaticEventType());
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_MouseMovedHandler, MouseMovedEvent), MouseMovedEvent::GetStaticEventType());
    }

    void Input::Shutdown()
    {
        g_KeyCodes.clear();

        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_KeyPressedHandler), KeyPressedEvent::GetStaticEventType());
        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_KeyReleasedHandler), KeyReleasedEvent::GetStaticEventType());
        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_ButtonPressedHandler), MouseButtonPressedEvent::GetStaticEventType());
        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_ButtonReleasedHandler), MouseButtonReleasedEvent::GetStaticEventType());
        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_MouseMovedHandler), MouseMovedEvent::GetStaticEventType());
    }

    bool Input::IsKeyPressed(u32 keyCode) const 
    {
        return g_KeyCodes[keyCode];
    }
    
    bool Input::IsMouseButtonPressed(u8 mouseButton) const
    {
        return g_MouseButtons[mouseButton];
    }

    std::pair<u32,u32> Input::GetMousePosition() const
    {
        return g_MousePos;
    }

    std::pair<u32,u32> Input::GetPreviousMousePosition() const
    {
        return g_PrevMousePos;
    }

    bool Input::OnKeyPressed(const KeyPressedEvent& e)
    {
        // only process event if key state has changed
        if(g_KeyCodes[e.GetKeyCode()] == false)
        {
            g_KeyCodes[e.GetKeyCode()] = true;
            //RAW_DEBUG("Key pressed: %u keycode", e.GetKeyCode());
        }
        return true;
    }

    bool Input::OnKeyReleased(const KeyReleasedEvent& e)
    {
        if(g_KeyCodes[e.GetKeyCode()] == true)
        {
            g_KeyCodes[e.GetKeyCode()] = false;
            //RAW_DEBUG("Key released: %u keycode", e.GetKeyCode());
        }
        return true;
    }

    bool Input::OnMouseButtonPressed(const MouseButtonPressedEvent& e)
    {
        // only process event if mouse button state has changed
        if(g_MouseButtons[e.GetMouseButton()] == false)
        {
            g_MouseButtons[e.GetMouseButton()] = true;
            //RAW_DEBUG("Mouse button pressed: %u button code", e.GetMouseButton());
        }
        return true;
    }

    bool Input::OnMouseButtonReleased(const MouseButtonReleasedEvent& e)
    {
        if(g_MouseButtons[e.GetMouseButton()] == true)
        {
            g_MouseButtons[e.GetMouseButton()] = false;
            //RAW_DEBUG("Mouse button released: %u button code", e.GetMouseButton());
        }
        return true;
    }

    bool Input::OnMouseMoved(const MouseMovedEvent& e)
    {
        if(e.GetMouseX() != g_MousePos.first || e.GetMouseY() != g_MousePos.second)
        {
            g_PrevMousePos = g_MousePos;
            g_MousePos.first = e.GetMouseX();
            g_MousePos.second = e.GetMouseY();
        }
        return true;
    }
}