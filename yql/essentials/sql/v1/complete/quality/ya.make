LIBRARY()

SRCS(
    keystroke_savings.cpp
)

PEERDIR(
    yql/essentials/sql/v1/complete
)

END()

RECURSE_FOR_TESTS(
    ut
)
