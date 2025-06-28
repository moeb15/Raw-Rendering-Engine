#pragma once

#include "core/defines.hpp"
#include "core/service.hpp"

namespace Raw
{
    class Timer : public IService
    {
    public:
        RAW_DECLARE_SERVICE(Timer);    

        void InitializeTimer();
        virtual void Shutdown() override {}
    
        i64 Now();
        
        f64 ToMicroseconds(i64 time);
        f64 ToMiliseconds(i64 time);
        f64 ToSeconds(i64 time);
    
        i64 From(i64 startingTime);
        i64 FromMicroseconds(i64 startingTime);
        i64 FromMiliseconds(i64 startingTime);
        i64 FromSeconds(i64 startingTime);
        
        f64 DeltaSeconds(i64 startingTime, i64 endingTime);
        f64 DeltaMiliseconds(i64 startingTime, i64 endingTime);
    };
}