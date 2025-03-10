#pragma once

#include "compute_storage.h"

#include <yql/essentials/minikql/computation/mkql_spiller_factory.h>
#include <contrib/ydb/library/yql/dq/actors/spilling/spilling_counters.h>

namespace NYql::NDq {

using namespace NActors;

class TDqSpillerFactory : public NKikimr::NMiniKQL::ISpillerFactory
{
public:
    TDqSpillerFactory(TTxId txId, TActorSystem* actorSystem, TWakeUpCallback wakeUpCallback, TErrorCallback errorCallback) 
        : ActorSystem_(actorSystem),
        TxId_(txId),
        WakeUpCallback_(wakeUpCallback),
        ErrorCallback_(errorCallback)
    {
    }

    void SetTaskCounters(const TIntrusivePtr<TSpillingTaskCounters>& spillingTaskCounters) override {
        SpillingTaskCounters_ = spillingTaskCounters;
    }

    NKikimr::NMiniKQL::ISpiller::TPtr CreateSpiller() override {
        return std::make_shared<TDqComputeStorage>(TxId_, WakeUpCallback_, ErrorCallback_, SpillingTaskCounters_, ActorSystem_);
    }

private:
    TActorSystem* ActorSystem_;
    TTxId TxId_;
    TWakeUpCallback WakeUpCallback_;
    TErrorCallback ErrorCallback_;
    TIntrusivePtr<TSpillingTaskCounters> SpillingTaskCounters_;
};

} // namespace NYql::NDq
