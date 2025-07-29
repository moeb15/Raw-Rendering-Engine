#include <iostream>
#include "core/logger.hpp"
#include "core/application.hpp"
#include "core/timer.hpp"
#include "memory/memory_service.hpp"

using namespace Raw;

int main()
{
    MemoryConfig config;
    config.maxSize = RAW_GB(1);

    MemoryService::Get()->Init(&config);
    Timer::Get()->InitializeTimer();
    Logger::Get()->InititalizeLogger();
    RAW_INFO("Logger Initialized.");
    Application* app = new Application();
    app->Initialize({ "Raw Renderer", 1600, 900 });
    app->Run();
    app->Shutdown();
    
    MemoryService::Get()->Shutdown();
    return 0;
}