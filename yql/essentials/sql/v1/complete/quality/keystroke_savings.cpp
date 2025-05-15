#include "metric.h"

#include <library/cpp/case_insensitive_string/case_insensitive_string.h>

namespace NSQLComplete {

    double EvaluateKeystrokeSavingsAscii(ISqlCompletionEngine& engine, TStringBuf text) {
        using TCIAStringBuf = TCaseInsensitiveAsciiStringBuf;

        const size_t keysNormal = text.size();
        size_t keysWithPrediction = 0;

        size_t i = 0;
        while (i < text.size()) {
            auto [token, candidates] = engine.Complete({.Text = text, .CursorPosition = i});
            if (candidates.size() == 1) {
                auto candidate = candidates[0];
                auto skip = candidate.Content.size() - token.Content.size();

                if (TCIAStringBuf expected = TStringBuf(text).substr(i - token.Content.length(), candidate.Content.length());
                    expected == TCIAStringBuf(candidate.Content)) {
                    i += skip;
                }
            }

            i += 1;
            keysWithPrediction += 1;
        }

        return static_cast<double>(keysNormal - keysWithPrediction) / keysNormal;
    }

} // namespace NSQLComplete
