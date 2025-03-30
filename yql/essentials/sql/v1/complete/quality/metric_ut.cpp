#include "metric.h"

#include <yql/essentials/sql/v1/complete/name/static/name_service.h>

#include <yql/essentials/sql/v1/lexer/lexer.h>
#include <yql/essentials/sql/v1/lexer/antlr4_pure/lexer.h>
#include <yql/essentials/sql/v1/lexer/antlr4_pure_ansi/lexer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NSQLComplete;

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

    Y_UNIT_TEST(SimpleQueryKeystrokeSavings) {
        auto engine = MakeSqlCompletionEngine(
            MakePureLexerSupplier(),
            MakeStaticNameService(MakeDefaultNameSet()));

        TString query = "SELECT 1 FROM a";
        size_t keysWithPrediction = (2 + 1) + (1) + 1 + (1 + 1) + 1;
        auto expected = static_cast<double>(query.size() - keysWithPrediction) / query.size();

        auto actual = EvaluateKeystrokeSavingsAscii(*engine, query);

        UNIT_ASSERT_DOUBLES_EQUAL(expected, actual, 0.01);
    }

    Y_UNIT_TEST(CoolQueryKeystrokeSavings) {
        auto engine = MakeSqlCompletionEngine(
            MakePureLexerSupplier(),
            MakeStaticNameService(MakeDefaultNameSet()));

        TString query = R"(
$json_parse = @@
    (json_string) -> (
        SELECT
            JSON_VALUE(json_string, '$.video_id') AS video_id,
            JSON_VALUE(json_string, '$.duration') AS duration,
            JSON_VALUE(json_string, '$.timestamp') AS timestamp
    )
@@;

SELECT
    user_id,
    video_id,
    MAX(CAST(duration AS Double)) AS total_watch_time_sec,
    COUNT(*) AS num_events,
    MAX(CAST(duration AS Double)) / MAX(video_meta.duration) * 100 AS completion_pct,
    IF(MAX(CAST(duration AS Double)) / MAX(video_meta.duration) < 25, 1, 0) AS is_early_dropoff,
    SUM(CAST(duration AS Double)) OVER (
        PARTITION BY user_id, video_id 
        ORDER BY CAST(timestamp AS Timestamp)
        ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    ) AS cumulative_watch_time
FROM 
    `video_events_raw`
    CROSS JOIN LATERAL $json_parse(event_data) AS parsed
    JOIN `video_metadata` AS video_meta 
        ON parsed.video_id = video_meta.video_id
WHERE 
    event_type = 'video_play'
    AND CAST(timestamp AS Timestamp) BETWEEN 
        DateTime::MakeDate(2023, 1, 1) AND CurrentUtcDate()
GROUP BY 
    user_id, video_id, timestamp
ORDER BY 
    total_watch_time_sec DESC
LIMIT 1000;
        )";

        double value = EvaluateKeystrokeSavingsAscii(*engine, query);
        UNIT_ASSERT_GT(value, 0.1);
    }
} // Y_UNIT_TEST_SUITE(MetricTests)
