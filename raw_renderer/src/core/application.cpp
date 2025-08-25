#include "core/application.hpp"
#include "core/servicelocator.hpp"
#include "core/asserts.hpp"
#include "platform/multiplatform_window.hpp"
#include "core/input.hpp"
#include "renderer/vulkan/vk_gfxdevice.hpp"
#include "events/event_manager.hpp"
#include "core/timer.hpp"
#include "core/job_system.hpp"
#include "resources/resource_manager.hpp"
#include "resources/texture_loader.hpp"
#include "resources/buffer_loader.hpp"
#include "scene/scene.hpp"
#include "renderer/renderer.hpp"
#include "core/keycodes.hpp"
#include "events/core_events.hpp"
#include "memory/smart_pointers.hpp"
#include "memory/memory_service.hpp"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

namespace Raw
{
    Scene* activeScene = nullptr;
    GFX::Renderer* activeRenderPath = nullptr;

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

        RAW_INFO("Initializing Input...");
        ServiceLocator::Get()->AddService(Input::Get(), Input::k_ServiceName);
        ServiceLocator::Get()->GetServiceByType<Input>()->InitializeInput();

        auto windowSize = MultiPlatformWindow::Get()->GetWindowSize();
        auto maxWindowSize = MultiPlatformWindow::Get()->GetMaxWindowSize();
        GFX::DeviceConfig dConfig;
        dConfig.name = config.name;
        dConfig.rendererAPI = GFX::EDeviceBackend::VULKAN;
        dConfig.width = windowSize.first;
        dConfig.height = windowSize.second;
        dConfig.maxWidth = maxWindowSize.first;
        dConfig.maxHeight = maxWindowSize.second;
        dConfig.windowHandle = MultiPlatformWindow::Get()->GetWindowHandle();

        RAW_INFO("Intializing Renderer Backend...");
        ServiceLocator::Get()->AddService(GFX::VulkanGFXDevice::Get(), GFX::IGFXDevice::k_ServiceName);
        ServiceLocator::Get()->GetServiceByType<GFX::VulkanGFXDevice>()->InitializeDevice(dConfig);
        ServiceLocator::Get()->GetServiceByType<GFX::VulkanGFXDevice>()->InitializeEditor();

        RAW_INFO("Initializing ResourceManager...");
        ResourceManager::Get()->Init();

        TextureLoader::Instance()->Init();
        BufferLoader::Instance()->Init();
        ResourceManager::Get()->SetLoader(TextureResource::k_ResourceType, TextureLoader::Instance());
        ResourceManager::Get()->SetLoader(BufferResource::k_ResourceType, BufferLoader::Instance());
        
        void* rndrData = RAW_ALLOCATE(sizeof(GFX::Renderer), alignof(GFX::Renderer));
        activeRenderPath = new (rndrData) GFX::Renderer();
        activeRenderPath->Init();
        
        GFX::IGFXDevice* device = (GFX::IGFXDevice*)ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName);
        void* sceneData = RAW_ALLOCATE(sizeof(Scene), alignof(Scene));
        activeScene = new (sceneData) Scene();

        static const std::string RAW_BASE_DIR{ BASE_DIR };
        std::string modelJSON = RAW_BASE_DIR + "/model.json";
        std::ifstream file(modelJSON);
        json jsonData = json::parse(file);

        auto it = jsonData.find("file");
        if(it == jsonData.end()) RAW_ASSERT_MSG(false, "Failed to parse model.json!");
        std::string modelRelPath = jsonData.value("file", "");
        std::string sceneModel = RAW_RESOURCES_DIR + modelRelPath.c_str();
        activeScene->Init(sceneModel, device);

        glm::vec3 pos(0.0f, 0.0f, 0.f);
        glm::vec3 target(0.0f,0.0f, -1.f);
        glm::vec3 up(0.f,1.f,0.0f);
        void* cmrData = RAW_ALLOCATE(sizeof(GFX::Camera), alignof(GFX::Camera));
        m_Camera = new (cmrData) GFX::Camera();
        m_Camera->Init(0.1f, 100.f, 75.f, 16.f / 9.f, pos, target, up);

        m_Suspended = false;
        file.close();
    }
    
    void Application::Run()
    {
        f64 runningTime = 0.0;
        i64 curTime = Timer::Get()->Now();
        i64 endTime = 0;
        f64 deltaTime = 1 / 60.0;

        while(ServiceLocator::Get()->GetServiceByType<MultiPlatformWindow>()->Update())
        {
            if(Input::Get()->IsKeyPressed(RAW_KEY_ESCAPE)) EventManager::Get()->TriggerEvent(rstd::make_unique<ApplicationExitEvent>());
           
            if(!m_Suspended)
            {
                m_Camera->Update(deltaTime);
                activeRenderPath->Render(activeScene, *m_Camera, deltaTime);

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
        activeRenderPath->Shutdown();
        activeScene->Shutdown();

        RAW_DEALLOCATE(activeRenderPath);
        RAW_DEALLOCATE(activeScene);
        RAW_DEALLOCATE(m_Camera);
        
        RAW_INFO("ResourceManager shutting down...");
        ResourceManager::Get()->Shutdown();
        ServiceLocator::Get()->GetService(Input::k_ServiceName)->Shutdown();
        ServiceLocator::Get()->GetService(GFX::IGFXDevice::k_ServiceName)->Shutdown();
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