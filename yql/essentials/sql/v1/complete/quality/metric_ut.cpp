#include "metric.h"

#include <yql/essentials/sql/v1/complete/name/cluster/static/discovery.h>
#include <yql/essentials/sql/v1/complete/name/object/dispatch/schema.h>
#include <yql/essentials/sql/v1/complete/name/object/simple/schema.h>
#include <yql/essentials/sql/v1/complete/name/object/simple/static/schema.h>
#include <yql/essentials/sql/v1/complete/name/service/ranking/frequency.h>
#include <yql/essentials/sql/v1/complete/name/service/ranking/ranking.h>
#include <yql/essentials/sql/v1/complete/name/service/cluster/name_service.h>
#include <yql/essentials/sql/v1/complete/name/service/schema/name_service.h>
#include <yql/essentials/sql/v1/complete/name/service/static/name_service.h>
#include <yql/essentials/sql/v1/complete/name/service/union/name_service.h>

#include <yql/essentials/sql/v1/lexer/lexer.h>
#include <yql/essentials/sql/v1/lexer/antlr4_pure/lexer.h>
#include <yql/essentials/sql/v1/lexer/antlr4_pure_ansi/lexer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/utf8.h>

using namespace NSQLComplete;

class TDummyNameService: public INameService {
public:
    TDummyNameService() {
        TVector<TString> lower;
        lower.reserve(Words_.size());
        for (const auto& word : Words_) {
            lower.emplace_back(ToLowerUTF8(word));
        }
        for (const auto& word : lower) {
            Words_.emplace_back(ToLowerUTF8(word));
        }
    }

    NThreading::TFuture<TNameResponse> Lookup(TNameRequest request) const override {
        TNameResponse response;
        for (const auto& word : Words_) {
            if (word.StartsWith(request.Prefix)) {
                TKeyword keyword;
                keyword.Content = word;
                response.RankedNames.emplace_back(std::move(keyword));
            }
        }
        return NThreading::MakeFuture(std::move(response));
    }

private:
    TVector<TString> Words_ = {
        "SELECT",
        "FROM",
        "WHERE",
        "GROUP",
        "ORDER",
        "BY",
        "LIMIT",
        "OFFSET",
        "EXPLAIN",
        "AST",
        "SET",
    };
};

Y_UNIT_TEST_SUITE(MetricTests) {
    TLexerSupplier MakePureLexerSupplier() {
        NSQLTranslationV1::TLexers lexers;
        lexers.Antlr4Pure = NSQLTranslationV1::MakeAntlr4PureLexerFactory();
        lexers.Antlr4PureAnsi = NSQLTranslationV1::MakeAntlr4PureAnsiLexerFactory();
        return [lexers = std::move(lexers)](bool ansi) {
            return NSQLTranslationV1::MakeLexer(
                lexers, ansi, /* antlr4 = */ true,
                NSQLTranslationV1::ELexerFlavor::Pure);
        };
    }

    ISqlCompletionEngine::TPtr MakeDummySqlCompletionEngine() {
        return MakeSqlCompletionEngine(
            MakePureLexerSupplier(),
            MakeIntrusive<TDummyNameService>());
    }

    ISqlCompletionEngine::TPtr MakeSqlCompletionEngineUT() {
        TLexerSupplier lexer = MakePureLexerSupplier();

        TNameSet names = {
            .Pragmas = {
                "yson.CastToString",
                "yt.RuntimeCluster",
                "yt.RuntimeClusterSelection",
            },
            .Types = {"Uint64"},
            .Functions = {
                "StartsWith",
                "DateTime::Split",
                "Python::__private",
            },
            .Hints = {
                {EStatementKind::Select, {"XLOCK"}},
                {EStatementKind::Insert, {"EXPIRATION"}},
            },
        };

        THashMap<TString, THashMap<TString, TVector<TFolderEntry>>> fss = {
            {"", {{"/", {{"Folder", "local"},
                         {"Folder", "test"},
                         {"Folder", "prod"},
                         {"Folder", ".sys"}}},
                  {"/local/", {{"Table", "example"},
                               {"Table", "account"},
                               {"Table", "abacaba"}}},
                  {"/test/", {{"Folder", "service"},
                              {"Table", "meta"}}},
                  {"/test/service/", {{"Table", "example"}}},
                  {"/.sys/", {{"Table", "status"}}}}},
            {"yt:saurus",
             {{"/", {{"Folder", "maxim"},
                     {"Table", "series"}}},
              {"/maxim/", {{"Table", "seasons"}}}}},
        };

        TVector<TString> clusters;
        for (const auto& [cluster, _] : fss) {
            clusters.emplace_back(cluster);
        }
        EraseIf(clusters, [](const auto& s) { return s.empty(); });

        TFrequencyData frequency;

        IRanking::TPtr ranking = MakeDefaultRanking(frequency);

        THashMap<TString, ISchema::TPtr> schemasByCluster;
        for (auto& [cluster, fs] : fss) {
            schemasByCluster[std::move(cluster)] =
                MakeSimpleSchema(
                    MakeStaticSimpleSchema(std::move(fs)));
        }

        TVector<INameService::TPtr> children = {
            MakeStaticNameService(std::move(names), frequency),
            MakeSchemaNameService(MakeDispatchSchema(std::move(schemasByCluster))),
            MakeClusterNameService(MakeStaticClusterDiscovery(std::move(clusters))),
        };

        INameService::TPtr service = MakeUnionNameService(std::move(children), ranking);

        return MakeSqlCompletionEngine(std::move(lexer), std::move(service));
    }

    static const TString SelectQuery = R"(
SELECT
    Re2::Capture("a.*")(sa.title),
    CAST(sr.series_id AS Uint64),
    sa.season_id
FROM
    yt:saurus.`/maxim/seasons` AS sa
INNER JOIN
    `/test/service/series` AS sr
ON sa.series_id = sr.series_id
WHERE sa.series_id = 1;
)";

    static const TString CreateQuery = R"(
CREATE TABLE `/test/service/series` (
    series_id Uint64,
    title Utf8,
    series_info Utf8,
    release_date Uint64,
    PRIMARY KEY (series_id)
);
)";

    Y_UNIT_TEST(SimpleQueryKeystrokeSavings) {
        auto engine = MakeSqlCompletionEngineUT();

        TString query = "SELECT 1 FROM a";
        size_t keysWithPrediction = (2 + 1) + (1) + 1 + (1 + 1) + 1;
        auto expected = static_cast<double>(query.size() - keysWithPrediction) / query.size();

        auto actual = EvaluateKeystrokeSavingsAscii(*engine, query);

        UNIT_ASSERT_DOUBLES_EQUAL(expected, actual, 0.01);
    }

    Y_UNIT_TEST(OldSelectKeystrokeSavings) {
        auto engine = MakeDummySqlCompletionEngine();

        TString query = SelectQuery;

        double value = EvaluateKeystrokeSavingsAscii(*engine, query);
        Cerr << "KS(Old, SELECT) = " << value << Endl;
    }

    Y_UNIT_TEST(OldCreateKeystrokeSavings) {
        auto engine = MakeDummySqlCompletionEngine();

        TString query = CreateQuery;

        double value = EvaluateKeystrokeSavingsAscii(*engine, query);
        Cerr << "KS(Old, CREATE) = " << value << Endl;
    }

    Y_UNIT_TEST(NewSelectKeystrokeSavings) {
        auto engine = MakeSqlCompletionEngineUT();

        TString query = SelectQuery;

        double value = EvaluateKeystrokeSavingsAscii(*engine, query);
        Cerr << "KS(New, SELECT) = " << value << Endl;
        UNIT_ASSERT_GT(value, 0.1);
    }

    Y_UNIT_TEST(NewCreateKeystrokeSavings) {
        auto engine = MakeSqlCompletionEngineUT();

        TString query = CreateQuery;

        double value = EvaluateKeystrokeSavingsAscii(*engine, query);
        Cerr << "KS(New, CREATE) = " << value << Endl;
        UNIT_ASSERT_GT(value, 0.1);
    }

} // Y_UNIT_TEST_SUITE(MetricTests)
