#pragma once

#include "local.h"
#include "token.h"

namespace NSQLComplete {

    TEditTextRange CurrentTokenEditTextRange(const TParsedTokenList& tokens, TCaretTokenPosition caret);

    TEditTextRange MultiTokenEditTextRange(
        const TParsedTokenList& tokens, TCaretTokenPosition caret, size_t beginIndex);

} // namespace NSQLComplete
