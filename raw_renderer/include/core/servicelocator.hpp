#pragma once

#include "core/defines.hpp"
#include "core/service.hpp"
#include <unordered_map>
#include <string>

namespace Raw
{
    class ServiceLocator
    {
    public:
        ServiceLocator() = default;
        ~ServiceLocator() {}
        DISABLE_COPY(ServiceLocator);

        static ServiceLocator* Get();
        
        void Shutdown();
        void AddService(IService* service, std::string name);
        IService* RemoveService(std::string name);
        IService* GetService(std::string name);

        template <typename T>
        T* GetServiceByType();

    private:
        std::unordered_map<std::string, IService*> m_Services;

    };

    template <typename T>
    RAW_INLINE T* ServiceLocator::GetServiceByType()
    {
        T* service = (T*)GetService(T::k_ServiceName);
        if(!service)
        {
            AddService(T::Get(), T::k_ServiceName);
        }

        return T::Get();
    }
}