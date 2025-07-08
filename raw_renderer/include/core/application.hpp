#pragma once

#include "core/defines.hpp"
#include "platform/window.hpp"
#include "events/event_handler.hpp"
#include "events/core_events.hpp"
#include "renderer/camera.hpp"
#include <string>
#include <memory>

namespace Raw
{
    struct ApplicationConfig
    {
        std::string name;
        u32 width;
        u32 height;
    };

    class GFX::Camera;

    class Application
    {
    public:
        Application() {}
        ~Application() {}

        DISABLE_COPY(Application);

        void Initialize(const ApplicationConfig& config);
        void Run();
        void Shutdown();

    private:
        bool m_Suspended;
    
        bool OnWindowMinimized(const WindowMinimizeEvent& e);
        bool OnWindowRestored(const WindowRestoredEvent& e);

        EventHandler<WindowMinimizeEvent> m_MinimizedHandler;
        EventHandler<WindowRestoredEvent> m_RestoredHandler;

        GFX::Camera* m_Camera;

    };
}