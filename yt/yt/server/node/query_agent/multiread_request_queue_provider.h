#pragma once

#include "public.h"

#include <yt/yt/core/rpc/public.h>

namespace NYT::NQueryAgent {

////////////////////////////////////////////////////////////////////////////////

NRpc::IRequestQueueProviderPtr CreateMultireadRequestQueueProvider();

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NQueryAgent
