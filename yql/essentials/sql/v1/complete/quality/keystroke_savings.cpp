#include "metric.h"

namespace NSQLComplete {

    double EvaluateKeystrokeSavingsAscii(ISqlCompletionEngine& engine, const TStringBuf text) {
        size_t keysNormal = text.size();
        size_t keysWithPrediction = 0;

        size_t i = 0;
        while (i < text.size()) {
            auto [token, candidates] = engine.Complete({.Text = text, .CursorPosition = i});
            if (candidates.size() == 1) {
                auto skip = candidates[0].Content.size() - token.Content.size();
                i += skip;
            }
            i += 1;
            keysWithPrediction += 1;
        }

        return static_cast<double>(keysNormal - keysWithPrediction) / keysNormal;
    }

} // namespace NSQLComplete
