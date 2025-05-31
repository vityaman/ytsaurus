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

        THashMap<TString, THashMap<TString, TVector<TFolderEntry>>> fss = {
            {"", {{"/", {{"Folder", "local"},
                         {"Folder", "test"},
                         {"Folder", "prod"},
                         {"Folder", ".sys"},
                         {"Table", "series"},
                         {"Table", "seasons"},
                         {"Table", "episodes"},
                         {"Table", "my_table"},
                         {"Table", "users"},
                         {"Table", "my_table_source"},
                        }},
                  {"/local/", {{"Table", "example"},
                               {"Table", "account"},
                               {"Table", "abacaba"}}},
                  {"/test/", {{"Folder", "service"},
                              {"Table", "meta"}}},
                  {"/test/service/", {{"Table", "example"}}},
                  {"/.sys/", {{"Table", "status"}}}}},
            {"yt:saurus",
             {{"/", {{"Folder", "maxim"},
                     {"Table", "series"},
                     {"Table", "series"},
                     {"Table", "seasons"},
                     {"Table", "episodes"},
                     {"Table", "my_table"},
                     {"Table", "users"},
                    }},
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
            MakeStaticNameService(MakeDefaultNameSet(), frequency),
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
    `series`_id Uint64,
    title Utf8,
    `series`_info Utf8,
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

    static const TString hugeQuery = R"(
CREATE TABLE `series`
(
    `series`_id Uint64,
    title Utf8,
    `series`_info Utf8,
    release_date Uint64,
    PRIMARY KEY (series_id)
);

CREATE TABLE `seasons`
(
    `series`_id Uint64,
    season_id Uint64,
    title Utf8,
    first_aired Uint64,
    last_aired Uint64,
    PRIMARY KEY (series_id, season_id)
);

CREATE TABLE `episodes`
(
    `series`_id Uint64,
    season_id Uint64,
    episode_id Uint64,
    title Utf8,
    air_date Uint64,
    PRIMARY KEY (series_id, season_id, episode_id)
);

COMMIT;

REPLACE INTO `series` (series_id, title, release_date, `series`_info) VALUES ();

SELECT * FROM `episodes`;

SELECT
    `series`_id,
    title AS `series`_title,
    CAST(release_date AS Date) AS release_date
FROM `series`;

SELECT
   `series`_id,
   season_id,
   episode_id,
   CAST(air_date AS Date) AS air_date,
   title
FROM `episodes`
WHERE
   `series`_id = 1
   AND season_id > 1
ORDER BY
   `series`_id,
   season_id,
   episode_id
LIMIT 3;

SELECT
    `series`_id,
    season_id,
    COUNT(*) AS cnt
FROM `episodes`
GROUP BY
    `series`_id,
    season_id
ORDER BY
    `series`_id,
    season_id;

SELECT
    `series`_title,
    String::JoinFromList(
        AGGREGATE_LIST(title),
        ","
    ) AS episode_titles
FROM `episodes`
WHERE `series`_id IN (1,2)
AND season_id = 1
GROUP BY
    CASE WHEN `series`_id = 1
         THEN "IT Crowd"
         ELSE "Other `series`"
    END AS `series`_title;

SELECT
    sa.title AS season_title,
    sr.title AS `series`_title,
    sr.series_id,
    sa.season_id
FROM `seasons` AS sa
INNER JOIN `series` AS sr
ON sa.series_id = sr.series_id
WHERE sa.series_id = 1
ORDER BY sr.series_id, sa.season_id;

SELECT * FROM `episodes` WHERE `series`_id = 2 AND season_id = 5;

SELECT * FROM `episodes` WHERE `series`_id = 2 AND season_id = 5;

$to_update = 
    SELECT `series`_id, season_id, episode_id,
           Utf8("Yesterday's Jam UPDATED") AS title
    FROM `episodes`
    WHERE `series`_id = 1 AND season_id = 1 AND episode_id = 1;

SELECT * FROM `episodes` WHERE `series`_id = 1 AND season_id = 1;

UPDATE `episodes` ON
SELECT * FROM $to_update;

DELETE
FROM `episodes`
WHERE
    `series`_id = 2
    AND season_id = 5
    AND episode_id = 12
;

ALTER TABLE `episodes` ADD COLUMN viewers Uint64;

ALTER TABLE `episodes` DROP COLUMN viewers;

DROP TABLE `episodes`;
DROP TABLE `seasons`;
DROP TABLE `series`;

SELECT COALESCE(
  maybe_empty_column,
  "it's empty!"
) FROM `my_table`;

$IsoParser = Udf(DateTime2::ParseIso8601);
SELECT $IsoParser("2022-01-01");

SELECT CurrentTzTimestamp("Europe/Moscow", TableRow()) FROM `my_table`;

SELECT CurrentLanguageVersion();

$var_type = Variant<foo: Int32, bar: Bool>;

SELECT key, COUNT(value) FROM `my_table` GROUP BY key;

SELECT
   AGGREGATE_LIST( region ),
   AGGREGATE_LIST( region, 5 ),
   AGGREGATE_LIST( DISTINCT region ),
   AGGREGATE_LIST_DISTINCT( region ),
   AGGREGATE_LIST_DISTINCT( region, 5 )
FROM `users`;

SELECT Histogram::Print(HISTOGRAM(numeric_column, 10), 50)
FROM `my_table`;

SELECT ListCreate(OptionalType(DataType("String")));

SELECT ListSortDesc(list_column) FROM `my_table`;

SELECT DictCreate(Tuple<Int32?,String>, OptionalType(DataType("String")));

SELECT InstanceOf(ParseType("Int32")) + 1.0;
SELECT FormatType(TypeOf(
    InstanceOf(ParseType("Int32")) +
    InstanceOf(ParseType("Double"))
));

INSERT INTO `my_table` WITH TRUNCATE
SELECT key FROM `my_table_source`;

SELECT * FROM (
    SELECT
        "1;2;3" AS a,
        AsList("x", "y", "z") AS b
) FLATTEN LIST BY (String::SplitToList(a, ";") as a, b);

SELECT x, y, z
FROM (
  SELECT
    AsStruct(
        1 AS x,
        "foo" AS y),
    AsStruct(
        false AS z)
) FLATTEN COLUMNS;

SELECT double_key, COUNT(*) FROM `my_table`
GROUP BY key + key AS double_key;

SELECT
    key
FROM `my_table`
GROUP BY key
HAVING COUNT(value) > 100;

DEFINE ACTION $hello() AS
    SELECT "Hello!";
END DEFINE;

DEFINE ACTION $bye() AS
    SELECT "Bye!";
END DEFINE;

EVALUATE IF RANDOM(0) > 0.5
    DO $hello()
ELSE
    DO $bye();

EVALUATE IF RANDOM(0) > 0.1 DO BEGIN
    SELECT "Hello!";
END DO;

EVALUATE FOR $i IN AsList(1, 2, 3) DO BEGIN
    SELECT $i;
END DO;

USE yt:saurus;

SELECT
    COUNT(*) OVER w AS rows_count_in_window,
    some_other_value
FROM `my_table`
WINDOW w AS (
    PARTITION BY partition_key_column
    ORDER BY int_column
);

SELECT
    column1,
    column2,
    column3,

    CASE GROUPING(
        column1,
        column2,
        column3,
    )
        WHEN 1  THEN "Subtotal: column1 and column2"
        WHEN 3  THEN "Subtotal: column1"
        WHEN 4  THEN "Subtotal: column2 and column3"
        WHEN 6  THEN "Subtotal: column3"
        WHEN 7  THEN "Grand total"
        ELSE         "Individual group"
    END AS subtotal,

    COUNT(*) AS rows_count

FROM `my_table`
)";

    Y_UNIT_TEST(OldHugeKeystrokeSavings) {
        auto engine = MakeDummySqlCompletionEngine();

        TString query = hugeQuery;

        double value = EvaluateKeystrokeSavingsAscii(*engine, query);
        Cerr << "KS(Old, Huge) = " << value << Endl;
    }

    Y_UNIT_TEST(NewHugeKeystrokeSavings) {
        auto engine = MakeSqlCompletionEngineUT();

        TString query = hugeQuery;

        double value = EvaluateKeystrokeSavingsAscii(*engine, query);
        Cerr << "KS(New, Huge) = " << value << Endl;
    }

} // Y_UNIT_TEST_SUITE(MetricTests)
