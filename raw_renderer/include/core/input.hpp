#pragma once

#include "core/defines.hpp"
#include "core/service.hpp"
#include "events/event_handler.hpp"
#include "events/core_events.hpp"
#include <utility>

namespace Raw
{
    class Input : public IService
    {
    public:
        RAW_DECLARE_SERVICE(Input)

        void InitializeInput();
        virtual void Shutdown() override;

        bool IsKeyPressed(u32 keyCode) const;
        bool IsMouseButtonPressed(u8 mouseButton) const;
        
        // Mouse position in the current frame
        std::pair<u32,u32> GetMousePosition() const;

        // Mouse position in the previous frame
        std::pair<u32,u32> GetPreviousMousePosition() const;
    
    private:
        bool OnKeyPressed(const KeyPressedEvent& e);
        bool OnKeyReleased(const KeyReleasedEvent& e);

        bool OnMouseButtonPressed(const MouseButtonPressedEvent& e);
        bool OnMouseButtonReleased(const MouseButtonReleasedEvent& e);
        bool OnMouseMoved(const MouseMovedEvent& e);

    private:
        EventHandler<KeyPressedEvent> m_KeyPressedHandler;  
        EventHandler<KeyReleasedEvent> m_KeyReleasedHandler;  
        EventHandler<MouseButtonPressedEvent> m_ButtonPressedHandler;  
        EventHandler<MouseButtonReleasedEvent> m_ButtonReleasedHandler;
        EventHandler<MouseMovedEvent> m_MouseMovedHandler;

    public:
        static constexpr cstring k_ServiceName = "raw_input_service";

    };
}