#include "metric.h"

#include <library/cpp/case_insensitive_string/case_insensitive_string.h>

#include <util/generic/map.h>

namespace NSQLComplete {

    double EvaluateKeystrokeSavingsAscii(ISqlCompletionEngine& engine, TStringBuf text) {
        Cerr << "[quality] Evaluating '" << text.Tail(1).Head(4) << "'" << Endl;

        using TCIAStringBuf = TCaseInsensitiveAsciiStringBuf;

        const size_t keysNormal = text.size();
        size_t keysWithPrediction = 0;

        TMap<ECandidateKind, size_t> skipsByKind;

        size_t i = 0;
        while (i < text.size()) {
            auto [token, candidates] = engine.Complete({.Text = text, .CursorPosition = i});
            if (candidates.size() == 1) {
                auto candidate = candidates[0];
                auto skip = candidate.Content.size() - token.Content.size();

                if (TCIAStringBuf expected = TStringBuf(text).substr(i - token.Content.length(), candidate.Content.length());
                    expected == TCIAStringBuf(candidate.Content)) {
                    // Cerr << "[quality] Accept '" << candidate.Content << "'" << Endl;
                    skipsByKind[candidate.Kind] += skip;
                    i += skip;
                }
            }

            i += 1;
            keysWithPrediction += 1;
        }

        for (auto [kind, skip] : skipsByKind) {
            Cerr << "[quality] Skipped " << skip << " by '" << kind << "'" << Endl;
        }

        Cerr << "[quality] KeysNormal: " << keysNormal << Endl;
        Cerr << "[quality] KeysWithPrediction: " << keysWithPrediction << Endl;

        return static_cast<double>(keysNormal - keysWithPrediction) / keysNormal;
    }

} // namespace NSQLComplete
