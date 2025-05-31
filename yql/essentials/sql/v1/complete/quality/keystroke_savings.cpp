#include "metric.h"

#include <library/cpp/case_insensitive_string/case_insensitive_string.h>

#include <util/generic/map.h>

namespace NSQLComplete {

    void RemoveSequencedWhitespaces(TString& s) {
    if (s.empty()) return;

    TString result;
    bool lastWasSpace = false;

    for (char c : s) {
        if (isspace(static_cast<unsigned char>(c))) {
            if (!lastWasSpace) {
                result += ' '; // replace any whitespace with single space
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }

    s = result;
}

    double EvaluateKeystrokeSavingsAscii(ISqlCompletionEngine& engine, TStringBuf t) {
        Cerr << "[quality] Evaluating '" << t.Tail(1).Head(4) << "'" << Endl;

        TString text(t);
        RemoveSequencedWhitespaces(text);

        TVector<TStringBuf> parts;
        Split(text, ";", parts);

        Cerr << "Queries: " << parts.size() << Endl;

        using TCIAStringBuf = TCaseInsensitiveAsciiStringBuf;

        const size_t keysNormal = text.size();
        size_t keysWithPrediction = 0;

        TMap<ECandidateKind, size_t> skipsByKind;
        size_t spaces = 0;

        for (auto part : parts) {

        size_t i = 0;
        while (i < part.size()) {
            auto [token, candidates] = engine.Complete({.Text = part, .CursorPosition = i});
            if (candidates.size() == 1) {
                auto candidate = candidates[0];
                auto skip = candidate.Content.size() - token.Content.size();

                if (TCIAStringBuf expected = TStringBuf(part).substr(i - token.Content.length(), candidate.Content.length());
                    expected == TCIAStringBuf(candidate.Content)) {
                    // Cerr << "[quality] Accept '" << candidate.Content << "'" << Endl;
                    skipsByKind[candidate.Kind] += skip;
                    i += skip;
                }
            }

            if (IsWhitespace(part[i])) {
                Y_ENSURE(i + 1 == parts.size() || !IsWhitespace(part[i + 1]));
                spaces += 1;
            }

            i += 1;
            keysWithPrediction += 1;
        }

        }

        for (auto [kind, skip] : skipsByKind) {
            Cerr << "[quality] Skipped " << skip << " by '" << kind << "'" << Endl;
        }

        Cerr << "[quality] KeysNormal: " << keysNormal << Endl;
        Cerr << "[quality] KeysWithPrediction: " << keysWithPrediction << Endl;
        Cerr << "[quality] Spaces: " << spaces << Endl;

        return static_cast<double>(keysNormal - keysWithPrediction) / keysNormal;
    }

} // namespace NSQLComplete
