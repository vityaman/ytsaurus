#pragma once

#include <yql/essentials/sql/v1/complete/core/statement.h>

#include <library/cpp/threading/future/core/future.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NSQLComplete {

    using NThreading::TFuture;

    struct TIndentifier {
        TString Indentifier;
    };

    struct TNamespaced {
        TString Namespace;
    };

    struct TKeyword {
        TString Content;
    };

    struct TPragmaName: TIndentifier {
        struct TConstraints: TNamespaced {};
    };

    struct TTypeName: TIndentifier {
        using TConstraints = std::monostate;
    };

    struct TFunctionName: TIndentifier {
        struct TConstraints: TNamespaced {};
    };

    struct THintName: TIndentifier {
        struct TConstraints {
            EStatementKind Statement;
        };
    };

    struct TFolderName: TIndentifier {
        struct TConstraints {};
    };

    struct TTableName: TIndentifier {
        struct TConstraints {};
    };

    struct TUnkownName {
        TString Content;
        TString Type;
    };

    using TGenericName = std::variant<
        TKeyword,
        TPragmaName,
        TTypeName,
        TFunctionName,
        THintName,
        TFolderName,
        TTableName,
        TUnkownName>;

    struct TNameRequest {
        TVector<TString> Keywords;
        struct {
            std::optional<TPragmaName::TConstraints> Pragma;
            std::optional<TTypeName::TConstraints> Type;
            std::optional<TFunctionName::TConstraints> Function;
            std::optional<THintName::TConstraints> Hint;
            std::optional<TFolderName::TConstraints> Folder;
            std::optional<TTableName::TConstraints> Table;
        } Constraints;
        TString Prefix = "";
        size_t Limit = 128;

        bool IsEmpty() const {
            return Keywords.empty() &&
                   !Constraints.Pragma &&
                   !Constraints.Type &&
                   !Constraints.Function &&
                   !Constraints.Hint &&
                   !Constraints.Folder &&
                   !Constraints.Table;
        }
    };

    struct TNameResponse {
        TVector<TGenericName> RankedNames;
        std::optional<size_t> NameHintLength;
    };

    class INameService: public TThrRefBase {
    public:
        using TPtr = TIntrusivePtr<INameService>;

        virtual TFuture<TNameResponse> Lookup(TNameRequest request) const = 0;
        virtual ~INameService() = default;
    };

} // namespace NSQLComplete
