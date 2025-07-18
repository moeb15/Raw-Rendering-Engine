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
    app->Initialize({ "Raw Renderer", 1600, 900 });
    app->Run();
    app->Shutdown();
    return 0;
}