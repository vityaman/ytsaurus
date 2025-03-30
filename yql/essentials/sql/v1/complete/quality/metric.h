#pragma once

#include <yql/essentials/sql/v1/complete/sql_complete.h>

#include <util/generic/string.h>

namespace NSQLComplete {

    double EvaluateKeystrokeSavingsAscii(ISqlCompletionEngine& engine, const TStringBuf text);

} // namespace NSQLComplete
