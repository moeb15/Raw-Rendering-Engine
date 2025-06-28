#pragma once

#include "core/defines.hpp"
#include "events/event.hpp"    

namespace Raw
{
    class ApplicationExitEvent : public IEvent
    {
    public:
        ApplicationExitEvent() {}
        virtual const u32 GetEventType() const override { return sk_EventType; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        static const u32 sk_EventType{ 0xab195a4c };

    };

    class WindowResizeEvent : public IEvent
    {
    public:
        WindowResizeEvent(u32 width, u32 height) : m_Width(width), m_Height(height) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        RAW_INLINE u32 GetWidth() const { return m_Width; }
        RAW_INLINE u32 GetHeight() const { return m_Height; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        u32 m_Width{ 0 };
        u32 m_Height{ 0 };
        static const u32 sk_EventType{ 0xa619abbe };

    };

    class WindowMinimizeEvent : public IEvent
    {
    public:
        WindowMinimizeEvent() {}
        virtual const u32 GetEventType() const override { return sk_EventType; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        static const u32 sk_EventType{ 0xdbbf5cb2 };

    };

    class WindowRestoredEvent : public IEvent
    {
    public:
        WindowRestoredEvent() {}
        virtual const u32 GetEventType() const override { return sk_EventType; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        static const u32 sk_EventType{ 0xa66ef287 };

    };

    class KeyPressedEvent : public IEvent
    {
    public:
        KeyPressedEvent(u32 keyCode) : m_KeyCode(keyCode) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        RAW_INLINE u32 GetKeyCode() const { return m_KeyCode; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        u32 m_KeyCode{ 0 };
        static const u32 sk_EventType{ 0xc5c835f1 };

    };

    class KeyReleasedEvent : public IEvent
    {
    public:
        KeyReleasedEvent(u32 keyCode) : m_KeyCode(keyCode) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        RAW_INLINE u32 GetKeyCode() const { return m_KeyCode; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        u32 m_KeyCode{ 0 };
        static const u32 sk_EventType{ 0xcf2157f8 };
    
    };

    class MouseMovedEvent : public IEvent
    {
    public:
        MouseMovedEvent(u32 x, u32 y) : m_X(x), m_Y(y) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        RAW_INLINE u32 GetMouseX() const { return m_X; }
        RAW_INLINE u32 GetMouseY() const { return m_Y; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        u32 m_X{ 0 };
        u32 m_Y{ 0 };
        static const u32 sk_EventType{ 0xa16c8e71 };
    
    };

    class MouseButtonPressedEvent : public IEvent
    {
    public:
        MouseButtonPressedEvent(u8 button) : m_Button(button) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        RAW_INLINE u8 GetMouseButton() const { return m_Button; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        u8 m_Button{ 0 };
        static const u32 sk_EventType{ 0xffedd723 };

    };

    class MouseButtonReleasedEvent : public IEvent
    {
    public:
        MouseButtonReleasedEvent(u8 button) : m_Button(button) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        RAW_INLINE u8 GetMouseButton() const { return m_Button; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        u8 m_Button{ 0 };
        static const u32 sk_EventType{ 0xde54b74f };

    };
}