#include "platform/multiplatform_window.hpp"
#include "core/asserts.hpp"
#include "events/event_manager.hpp"
#include "events/core_events.hpp"
#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>

namespace Raw
{
    static MultiPlatformWindow s_Window;
    static SDL_Rect windowRect = {};
    
    MultiPlatformWindow* MultiPlatformWindow::Get()
    {
        return &s_Window;
    }

    bool MultiPlatformWindow::Initialize(const WindowConfig& config)
    {
        RAW_INFO("Initializing MultiPlatformWindow...");

        SDL_WindowFlags flags = 0;
        flags |= SDL_WINDOW_VULKAN;
        flags |= SDL_WINDOW_RESIZABLE;
        //flags |= SDL_WINDOW_FULLSCREEN;

        m_pWindow = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
        RAW_ASSERT_MSG(m_pWindow != nullptr, "Failed to create SDL window!");

        m_Running = true;

        m_ExitHandler = BIND_EVENT_FN(MultiPlatformWindow::OnApplicationExit);
        EventManager::Get()->Subscribe(EVENT_HANDLER_PTR(m_ExitHandler, ApplicationExitEvent), ApplicationExitEvent::GetStaticEventType());

        SDL_DisplayID id = SDL_GetDisplayForWindow(m_pWindow);
        SDL_GetDisplayBounds(id, &windowRect);

        return true;
    }
    
    void MultiPlatformWindow::Shutdown()
    {
        RAW_INFO("MultiPlatformWindow shutting down...");

        SDL_DestroyWindow(m_pWindow);
    }
    
    bool MultiPlatformWindow::Update()
    {
        SDL_Event e;
        while(SDL_PollEvent(&e) != 0)
        {
            ImGui_ImplSDL3_ProcessEvent(&e);
            
            if(e.type == SDL_EVENT_QUIT)
            {
                // Immediately trigger application exit event so listeners can respond
                EventManager::Get()->TriggerEvent(rstd::make_unique<ApplicationExitEvent>());
                break;
            }

            // Immediately trigger window events
            if (e.type == SDL_EVENT_WINDOW_RESIZED)          EventManager::Get()->TriggerEvent(rstd::make_unique<WindowResizeEvent>((u32)e.window.data1, (u32)e.window.data2));
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED)        EventManager::Get()->TriggerEvent(rstd::make_unique<WindowMinimizeEvent>());
            if (e.type == SDL_EVENT_WINDOW_RESTORED)         EventManager::Get()->TriggerEvent(rstd::make_unique<WindowRestoredEvent>());

            if (e.type == SDL_EVENT_KEY_DOWN)                EventManager::Get()->QueueEvent(rstd::make_unique<KeyPressedEvent>((u32)e.key.key));
            if (e.type == SDL_EVENT_KEY_UP)                  EventManager::Get()->QueueEvent(rstd::make_unique<KeyReleasedEvent>((u32)e.key.key));
            if (e.type == SDL_EVENT_MOUSE_MOTION)            EventManager::Get()->QueueEvent(rstd::make_unique<MouseMovedEvent>((u32)e.motion.x, (u32)e.motion.y));
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)       EventManager::Get()->QueueEvent(rstd::make_unique<MouseButtonPressedEvent>((u32)e.button.button));
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)         EventManager::Get()->QueueEvent(rstd::make_unique<MouseButtonReleasedEvent>((u32)e.button.button));
        }

        return m_Running;
    }

    std::pair<u32, u32> MultiPlatformWindow::GetWindowSize()
    {
        int x, y;
        SDL_GetWindowSize(m_pWindow, &x, &y);

        return std::pair<u32,u32>((u32)x, (u32)y);
    }

    std::pair<u32, u32> MultiPlatformWindow::GetMaxWindowSize()
    {
        return std::pair<u32,u32>((u32)windowRect.w, (u32)windowRect.h);
    }

    bool MultiPlatformWindow::OnApplicationExit(const ApplicationExitEvent& e)
    {
        RAW_INFO("Exitting application...");
        m_Running = false;
        return true;
    }
}