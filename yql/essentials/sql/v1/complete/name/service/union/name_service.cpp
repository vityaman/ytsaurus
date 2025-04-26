#include "name_service.h"

#include <library/cpp/threading/future/wait/wait.h>

namespace NSQLComplete {

    namespace {

        class TNameService: public INameService {
        public:
            explicit TNameService(TVector<INameService::TPtr> children)
                : Children_(std::move(children))
            {
                Y_ENSURE(!Children_.empty());
            }

            TFuture<TNameResponse> Lookup(TNameRequest request) const override {
                TVector<TFuture<TNameResponse>> children;
                children.reserve(Children_.size());
                for (const auto& child : Children_) {
                    children.emplace_back(child->Lookup(request));
                }

                return NThreading::WaitAll(children).Apply([children = std::move(children)](auto) {
                    TNameResponse response;
                    for (auto f : children) {
                        TNameResponse child = f.ExtractValue();
                        if (child.NameHintLength) {
                            Y_ENSURE(!response.NameHintLength);
                            response.NameHintLength = child.NameHintLength;
                        }
                        std::ranges::move(
                            std::move(child.RankedNames),
                            std::back_inserter(response.RankedNames));
                    }
                    return response;
                });
            }

        private:
            TVector<INameService::TPtr> Children_;
        };

    } // namespace

    INameService::TPtr MakeUnionNameService(TVector<INameService::TPtr> children) {
        return INameService::TPtr(new TNameService(std::move(children)));
    }

} // namespace NSQLComplete
