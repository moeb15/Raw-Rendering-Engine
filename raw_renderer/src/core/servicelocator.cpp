#include "core/servicelocator.hpp"
#include "core/logger.hpp"
#include <utility>

namespace Raw
{
    static ServiceLocator s_ServiceLocator;

    ServiceLocator* ServiceLocator::Get()
    {
        return &s_ServiceLocator;
    }
        
    void ServiceLocator::Shutdown()
    {
        RAW_INFO("ServiceLocator shutting down...");
        m_Services.clear();
    }
    
    void ServiceLocator::AddService(IService* service, std::string name)
    {
        if(m_Services.find(name) != m_Services.end())
        {
            RAW_WARN("Service '%s' already present.", name.c_str());
            return;
        }

        m_Services.insert(std::pair<std::string, IService*>(name, service));
    }
    
    IService* ServiceLocator::RemoveService(std::string name)
    {
        if(m_Services.find(name) == m_Services.end())
        {
            RAW_WARN("Service '%s' is not present.", name.c_str());
            return nullptr;
        }
        IService* servicePtr = m_Services[name];
        m_Services.erase(name);

        RAW_DEBUG("Service '%s' has been removed.", name.c_str());

        return servicePtr;
    }
    
    IService* ServiceLocator::GetService(std::string name)
    {
        if(m_Services.find(name) == m_Services.end())
        {
            RAW_WARN("Service '%s' is not present.", name.c_str());
            return nullptr;
        }

        return m_Services[name];
    }
}