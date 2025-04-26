LIBRARY()

SRCS(
    frequency.cpp
    ranking.cpp
)

PEERDIR(
    yql/essentials/core/sql_types
    yql/essentials/sql/v1/complete/name/service
    yql/essentials/sql/v1/complete/text
)

RESOURCE(
    yql/essentials/data/language/rules_corr_basic.json rules_corr_basic.json
)

END()

RECURSE_FOR_TESTS(
    ut
)
