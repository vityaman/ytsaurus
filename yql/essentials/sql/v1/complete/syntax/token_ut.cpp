#include "token.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NSQLComplete;

Y_UNIT_TEST_SUITE(CaretTokenPositionTests) {
    TParsedTokenList From(TVector<TString> contents) {
        TParsedTokenList list;
        list.reserve(contents.size());
        for (auto& content : contents) {
            NSQLTranslation::TParsedToken token = {
                .Content = std::move(content),
            };
            list.emplace_back(std::move(token));
        }
        return list;
    }

    Y_UNIT_TEST(Blank) {
        {
            auto pos = CaretTokenPosition(From({}), 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.NextTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenPosition, 0);
        }
        {
            auto pos = CaretTokenPosition(From({" "}), 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.NextTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenPosition, 0);
        }
        {
            auto pos = CaretTokenPosition(From({" "}), 1);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.NextTokenIndex, 1);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenPosition, 0);
        }
    }

    Y_UNIT_TEST(Enslosed) {
        {
            auto pos = CaretTokenPosition(From({"SELECT", " ", "`test`"}), 10);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenIndex, 2);
            UNIT_ASSERT_VALUES_EQUAL(pos.NextTokenIndex, 2);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenPosition, 7);
        }
        {
            auto pos = CaretTokenPosition(From({"SELECT", " ", "`test`"}), 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.NextTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenPosition, 0);
        }
    }

    Y_UNIT_TEST(Between) {
        {
            auto pos = CaretTokenPosition(From({"lhs", "`rhs`"}), 3);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.NextTokenIndex, 1);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenPosition, 0);
        }
        {
            auto pos = CaretTokenPosition(From({"`lhs`", "`rhs`"}), 5);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenIndex, 0);
            UNIT_ASSERT_VALUES_EQUAL(pos.NextTokenIndex, 1);
            UNIT_ASSERT_VALUES_EQUAL(pos.PrevTokenPosition, 0);
        }
    }
} // Y_UNIT_TEST_SUITE(CaretTokenPositionTests)
