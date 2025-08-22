#pragma once

#include "ecs/component.hpp"
#include "core/asserts.hpp"
#include "containers/vector.hpp"
// temporary
#include <unordered_map>

namespace Raw
{
    class IComponentManager
    {
    public:
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        virtual void Remove(Entity e) = 0;
        virtual bool Contains(Entity e) const = 0;
        virtual u32 GetCount() const = 0;
    };

    template <typename T>
    class ComponentManager : public IComponentManager
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;
        
        virtual bool Contains(Entity e) const override;
        virtual void Remove(Entity e) override;
        T& Add(Entity e);

        RAW_INLINE T* GetComponent(Entity e)
        {
            if(Contains(e))
            {
                u32 index = m_LookUp[e];
                return &m_Components[index];
            }
            return nullptr;
        }

        RAW_INLINE virtual u32 GetCount() const override { return m_Components.size(); }
        RAW_INLINE Entity GetEntity(u32 index) const { return m_Entities[index]; }

        RAW_INLINE T& operator[](u32 index) { return m_Components[index]; }
        RAW_INLINE const T& operator[](u32 index) const { return m_Components[index]; }

    private:
        rstd::vector<T> m_Components;
        rstd::vector<Entity> m_Entities;
        std::unordered_map<Entity, u32> m_LookUp;

    };

    template <typename T>
    void ComponentManager<T>::Init()
    {
        m_Components.reserve(5);
        m_Entities.reserve(5);
    }

    template <typename T>
    void ComponentManager<T>::Shutdown()
    {
        m_Components.shutdown();
        m_Entities.shutdown();
        m_LookUp.clear();
    }

    template <typename T>
    bool ComponentManager<T>::Contains(Entity e) const
    {
        return m_LookUp.find(e) != m_LookUp.end();
    }

    template <typename T>
    T& ComponentManager<T>::Add(Entity e)
    {
        RAW_ASSERT(e != INVALID_ENTITY_ID);
        RAW_ASSERT(m_LookUp.find(e) == m_LookUp.end());
        RAW_ASSERT(m_Entities.size() == m_Components.size());
        RAW_ASSERT(m_Components.size() == m_LookUp.size());

        m_LookUp[e] = m_Components.size();
        m_Components.push_back(T());
        m_Entities.push_back(e);

        return m_Components.back();
    }

    template <typename T>
    void ComponentManager<T>::Remove(Entity e)
    {
        auto it = m_LookUp.find(e);
        if(it != m_LookUp.end())
        {
            const u32 index = it->second;
            const Entity lastEntity = m_Entities.back();

            m_Components[index] = std::move(m_Components.back());
            m_Entities[index] = lastEntity;

            m_Components.pop_back();
            m_Entities.pop_back();
            m_LookUp.erase(e);
        }
    }
}