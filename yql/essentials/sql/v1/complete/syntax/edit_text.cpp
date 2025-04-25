#include "edit_text.h"

#include <yql/essentials/sql/v1/complete/text/word.h>

namespace NSQLComplete {

    TEditTextRange CurrentTokenEditTextRange(const TParsedTokenList& tokens, TCaretTokenPosition caret) {
        if (caret.Position == 0 || tokens.size() <= caret.PrevTokenIndex) {
            return {.Begin = caret.Position};
        }

        const TString& prevContent = tokens.at(caret.PrevTokenIndex).Content;
        if (caret.PrevTokenPosition == caret.NextTokenIndex) {
            return {
                .Begin = caret.PrevTokenPosition,
                .End = prevContent.size(),
            };
        }

        if (IsWordBoundary(prevContent.back())) {
            return {
                .Begin = caret.Position,
            };
        }

        return {
            .Begin = caret.PrevTokenPosition,
            .End = caret.PrevTokenPosition + prevContent.size(),
        };
    }

    TEditTextRange MultiTokenEditTextRange(
        const TParsedTokenList& tokens, TCaretTokenPosition caret, size_t beginIndex) {
        TEditTextRange range;
        range.End = caret.PrevTokenPosition + tokens.at(caret.PrevTokenIndex).Content.size();

        size_t contentLength = 0;
        for (size_t i = beginIndex; i <= caret.PrevTokenIndex; ++i) {
            contentLength += tokens[i].Content.size();
        }

        range.Begin = range.End - contentLength;

        return range;
    }

} // namespace NSQLComplete
