#include <iostream>
#include "core/logger.hpp"
#include "core/application.hpp"
#include "core/timer.hpp"

using namespace Raw;

int main()
{
    Timer::Get()->InitializeTimer();
    Logger::Get()->InititalizeLogger();
    RAW_INFO("Logger Initialized.");
    Application* app = new Application();
    app->Initialize({ "Raw Renderer", 1280, 720 });
    app->Run();
    app->Shutdown();
    return 0;
}