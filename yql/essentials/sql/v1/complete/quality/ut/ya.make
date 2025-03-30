UNITTEST_FOR(yql/essentials/sql/v1/complete/quality)

SRCS(
    metric_ut.cpp
)

PEERDIR(
    yql/essentials/sql/v1/lexer
    yql/essentials/sql/v1/lexer/antlr4_pure
    yql/essentials/sql/v1/lexer/antlr4_pure_ansi

    yql/essentials/sql/v1/complete/name/static
)

END()
