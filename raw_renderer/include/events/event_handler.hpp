#pragma once

#include "core/defines.hpp"
#include "events/event.hpp"
#include "memory/smart_pointers.hpp"
#include <functional>
#include <string>

#define GET_HANDLER_NAME(handler) handler.target_type().name()
#define EVENT_HANDLER_PTR(handler, event) rstd::make_unique<EventHandlerWrapper<event>>(handler)

namespace Raw
{
    template <typename EventType>
    using EventHandler = std::function<bool(const EventType& e)>;

    class IEventHandlerWrapper
    {
    public:
        bool Execute(const IEvent& e)
        {
            return Call(e);
        }

        virtual std::string GetType() const = 0;

    private:
        virtual bool Call(const IEvent& e) = 0;

    };
    
    template <typename EventType>
    class EventHandlerWrapper : public IEventHandlerWrapper
    {
    public:
        EventHandlerWrapper(const EventHandler<EventType>& handler) :
            m_Handler(handler),
            m_HandlerType(m_Handler.target_type().name()) {}

        virtual std::string GetType() const override { return m_HandlerType; }

    private:
        virtual bool Call(const IEvent& e) override
        {
            if(e.GetEventType() == EventType::GetStaticEventType())
            {
                return m_Handler(static_cast<const EventType&>(e));
            }
            return false;
        }

    private:
        EventHandler<EventType> m_Handler;
        const std::string m_HandlerType;
    };
}