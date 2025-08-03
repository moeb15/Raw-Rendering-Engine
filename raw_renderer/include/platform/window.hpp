#pragma once

#include "core/defines.hpp"
#include "core/service.hpp"
#include "core/string.hpp"
// #include <string>
#include <utility>


namespace Raw
{
    struct WindowConfig
    {
        rstd::string title;
        u32 width;
        u32 height;
    };

    class IWindow : public IService
    {
    public:
        virtual bool Initialize(const WindowConfig& config) = 0;
        virtual bool Update() = 0;
        virtual void* GetWindowHandle() = 0;
        virtual std::pair<u32, u32> GetWindowSize() = 0;
        virtual std::pair<u32, u32> GetMaxWindowSize() = 0;
        
        static constexpr cstring k_ServiceName = "raw_window_service";

    };
}