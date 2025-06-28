#pragma once

#include "events/event_handler.hpp"
#include <list>
#include <map>

namespace Raw
{
    constexpr u32 EVENT_MANAGER_QUEUE_COUNT = 2;

    class EventManager
    {
    private:
        typedef std::unique_ptr<IEvent> IEventPtr;
        typedef std::unique_ptr<IEventHandlerWrapper> EventHandlerPtr;
        typedef std::list<EventHandlerPtr> EventHandlerList;
        typedef std::list<IEventPtr> EventQueue;
        typedef std::map<u32, EventHandlerList> EventHandlerMap;

    public:    
        static EventManager* Get();

        bool Initialize();
        void Shutdown();

        void Subscribe(EventHandlerPtr&& handler, const u32& eventType);
        void Unsubscribe(const std::string handlerType, const u32& eventType);
        void TriggerEvent(IEventPtr&& event);
        void QueueEvent(IEventPtr&& event);

        bool ProcessEvents();
    
    private:
        EventHandlerMap m_EventHandlers;
        EventQueue m_EventQueues[EVENT_MANAGER_QUEUE_COUNT];
        u32 m_ActiveQueue;
    };
}