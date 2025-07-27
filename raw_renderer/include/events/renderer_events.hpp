#pragma once

#include "core/defines.hpp"
#include "events/event.hpp"    

namespace Raw
{
    class AOToggledEvent : public IEvent
    {
    public:
        AOToggledEvent(bool flag) : m_Toggle(flag) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        bool GetState() const { return m_Toggle; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        bool m_Toggle{ false };
        static const u32 sk_EventType{ 0xd59d83e5 };

    };

    class ReflectionsToggledEvent : public IEvent
    {
    public:
        ReflectionsToggledEvent(bool flag) : m_Toggle(flag) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        bool GetState() const { return m_Toggle; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        bool m_Toggle{ false };
        static const u32 sk_EventType{ 0xf0f1db82 };

    };

    class AntiAliasingToggledEvent : public IEvent
    {
    public:
        AntiAliasingToggledEvent(bool flag) : m_Toggle(flag) {}
        virtual const u32 GetEventType() const override { return sk_EventType; }
        bool GetState() const { return m_Toggle; }

        static u32 GetStaticEventType() { return sk_EventType; }

    private:
        bool m_Toggle{ false };
        static const u32 sk_EventType{ 0xff45fb5d };

    };
}