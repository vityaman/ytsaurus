#pragma once

#include <yql/essentials/sql/v1/complete/sql_complete.h>

#include <yql/essentials/sql/v1/lexer/lexer.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

namespace NSQLComplete {

    struct TEditTextRange {
        size_t Begin = 0;
        size_t End = Begin;
    };

    struct TLocalSyntaxContext {
        using TKeywords = THashMap<TString, TVector<TString>>;

        struct TPragma {
            TString Namespace;
        };

        struct TFunction {
            TString Namespace;
        };

        struct THint {
            EStatementKind StatementKind;
        };

        struct TObject {
            enum class EKind {
                Folder,
                Table,
            };

            TString Path;
            THashSet<EKind> Kinds;
        };

        TKeywords Keywords;
        std::optional<TPragma> Pragma;
        bool IsTypeName = false;
        std::optional<TFunction> Function;
        std::optional<THint> Hint;
        std::optional<TObject> Object;
        TEditTextRange EditRange;
        bool IsEnclosed = false;
    };

    class ILocalSyntaxAnalysis {
    public:
        using TPtr = THolder<ILocalSyntaxAnalysis>;

        virtual TLocalSyntaxContext Analyze(TCompletionInput input) = 0;
        virtual ~ILocalSyntaxAnalysis() = default;
    };

    ILocalSyntaxAnalysis::TPtr MakeLocalSyntaxAnalysis(TLexerSupplier lexer);

} // namespace NSQLComplete
