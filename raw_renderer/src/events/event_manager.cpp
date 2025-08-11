#include "events/event_manager.hpp"
#include "core/asserts.hpp"

namespace Raw
{
    static EventManager s_EventManager;
    static bool isInitialized = false;

    EventManager* EventManager::Get()
    {
        return &s_EventManager;
    }

    bool EventManager::Initialize()
    {
        RAW_ASSERT_MSG(isInitialized == false, "EventManager subsystem already initialized!");
        RAW_INFO("Intitializing EventManager...");
        
        m_ActiveQueue = 0;
        isInitialized = true;

        return true;
    }

    void EventManager::Shutdown()
    {
        RAW_INFO("EventManager shutting down...");
        m_EventHandlers.clear();
        for(u32 i = 0; i < EVENT_MANAGER_QUEUE_COUNT; i++)
        {
            m_EventQueues[i].clear();
        }
        m_ActiveQueue = 0;
    }

    void EventManager::Subscribe(EventHandlerPtr&& handler, const u32& eventType)
    {
        auto subscribers = m_EventHandlers.find(eventType);
        if(subscribers != m_EventHandlers.end())
        {
            EventHandlerList& handlers = subscribers->second;
            for(auto it = handlers.begin(); it != handlers.end(); it++)
            {
                RAW_ASSERT_MSG(it->get()->GetType() != handler->GetType(), "Attempting to double-register callback!");
                //return;
            }
            handlers.emplace_back(std::move(handler));
        }
        else
        {
            m_EventHandlers[eventType].emplace_back(std::move(handler));
        }
    }

    void EventManager::Unsubscribe(const std::string handlerType, const u32& eventType)
    {
        auto subscribers = m_EventHandlers.find(eventType);
        if(subscribers != m_EventHandlers.end())
        {
            EventHandlerList& handlers = subscribers->second;
            for(auto it = handlers.begin(); it != handlers.end(); it++)
            {
                if(it->get()->GetType() == handlerType)
                {
                    it = handlers.erase(it);
                    RAW_DEBUG("Handler '%s' unsubscribed from event %u", handlerType.c_str(), eventType);
                    return;
                }
            }
        }

        RAW_WARN("Handler '%s' was not subscribed to event %u", handlerType.c_str(), eventType);
    }
    
    void EventManager::TriggerEvent(IEventPtr&& event)
    {
        for(auto& handler : m_EventHandlers[event->GetEventType()])
        {
            event->m_Handled = handler->Execute(*event.get());
            if(event->m_Handled) break;
        }
    }
    
    void EventManager::QueueEvent(IEventPtr&& event)
    {
        RAW_ASSERT(m_ActiveQueue >= 0);
        RAW_ASSERT(m_ActiveQueue < EVENT_MANAGER_QUEUE_COUNT);

        auto it = m_EventHandlers.find(event->GetEventType());
        if(it != m_EventHandlers.end())
        {
            m_EventQueues[m_ActiveQueue].push_back(std::move(event));
            return;
        }
    }

    bool EventManager::ProcessEvents()
    {
        u32 queueToProcess = m_ActiveQueue;
        m_ActiveQueue = (m_ActiveQueue + 1) % EVENT_MANAGER_QUEUE_COUNT;
        m_EventQueues[m_ActiveQueue].clear();

        while(!m_EventQueues[queueToProcess].empty())
        {
            IEventPtr pEvent = std::move(m_EventQueues[queueToProcess].front());
            m_EventQueues[queueToProcess].pop_front();

            const u32& eventType = pEvent->GetEventType();

            auto subscribers = m_EventHandlers.find(eventType);
            if(subscribers != m_EventHandlers.end())
            {
                EventHandlerList& handlers = subscribers->second;
                for(auto& it : handlers)
                {
                    pEvent->m_Handled = it->Execute(*pEvent.get());
                    if(pEvent->m_Handled) break;
                }
            }
        }

        return true;
    }
}