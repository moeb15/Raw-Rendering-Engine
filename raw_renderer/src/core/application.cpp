#include "core/application.hpp"
#include "core/servicelocator.hpp"
#include "core/asserts.hpp"
#include "platform/multiplatform_window.hpp"
//#include "core/input.hpp"
#include "events/event_manager.hpp"
#include "core/timer.hpp"
#include "core/job_system.hpp"

namespace Raw
{
    void Application::Initialize(const ApplicationConfig& config)
    {
        RAW_INFO("Initializing Application...");

        WindowConfig wConfig;
        wConfig.title = config.name;
        wConfig.width = config.width;
        wConfig.height = config.height;

        RAW_INFO("Initializing JobSystem...");
        JobSystem::Init();
        
        RAW_ASSERT_MSG(EventManager::Get()->Initialize(), "Failed to initialize EventManager");

        // event handler, when window is minimized suspend the game loop
        m_MinimizedHandler = BIND_EVENT_FN(Application::OnWindowMinimized);
        m_RestoredHandler = BIND_EVENT_FN(Application::OnWindowRestored);

        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_MinimizedHandler, WindowMinimizeEvent), WindowMinimizeEvent::GetStaticEventType());
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_RestoredHandler, WindowRestoredEvent), WindowRestoredEvent::GetStaticEventType());

        ServiceLocator::Get()->AddService(MultiPlatformWindow::Get(), IWindow::k_ServiceName);
        ServiceLocator::Get()->GetServiceByType<MultiPlatformWindow>()->Initialize(wConfig);

        m_Suspended = false;
    }
    
    void Application::Run()
    {
        f64 runningTime = 0.0;
        i64 curTime = Timer::Get()->Now();
        i64 endTime = 0;
        f64 deltaTime = 1 / 60.0;

        while(ServiceLocator::Get()->GetServiceByType<MultiPlatformWindow>()->Update())
        {
            if(!m_Suspended)
            {
                endTime = Timer::Get()->Now();
                deltaTime = Timer::Get()->DeltaSeconds(curTime, endTime);
                curTime = endTime;

                if(!EventManager::Get()->ProcessEvents())
                {
                    RAW_TRACE("Could not process all events in queue!");
                }
            }
        }
    }
    
    void Application::Shutdown()
    {
        ServiceLocator::Get()->GetService(IWindow::k_ServiceName)->Shutdown();
     
        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_MinimizedHandler), WindowMinimizeEvent::GetStaticEventType());
        EventManager::Get()->Unsubscribe(GET_HANDLER_NAME(m_RestoredHandler), WindowRestoredEvent::GetStaticEventType());
     
        EventManager::Get()->Shutdown();
        ServiceLocator::Get()->Shutdown();

        RAW_INFO("Application shutting down...");
    }

    bool Application::OnWindowMinimized(const WindowMinimizeEvent& e)
    {
        m_Suspended = true;
        RAW_DEBUG("Application suspended.");
        return true;
    }
    
    bool Application::OnWindowRestored(const WindowRestoredEvent& e)
    {
        m_Suspended = false;
        RAW_DEBUG("Application restored.");
        return true;
    }
}