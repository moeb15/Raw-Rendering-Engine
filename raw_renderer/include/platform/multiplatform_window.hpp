#pragma once

#include "platform/window.hpp"
#include "events/event_handler.hpp"
#include "events/core_events.hpp"

struct SDL_Window;

namespace Raw
{
    class MultiPlatformWindow : public IWindow
    {
    public:
        RAW_DECLARE_SERVICE(MultiPlatformWindow);
        DISABLE_COPY(MultiPlatformWindow);
        
        MultiPlatformWindow() {};
        ~MultiPlatformWindow() {};

        virtual bool Initialize(const WindowConfig& config) override;
        virtual void Shutdown() override;
        virtual bool Update() override;
        virtual void* GetWindowHandle() override { return m_pWindow; }
        virtual std::pair<u32, u32> GetWindowSize() override;
        virtual std::pair<u32, u32> GetMaxWindowSize() override;

    private:
        bool OnApplicationExit(const ApplicationExitEvent& e);

    private:
        SDL_Window* m_pWindow;
        EventHandler<ApplicationExitEvent> m_ExitHandler;
        bool m_Running;

    };
}