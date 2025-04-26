#include "name_service.h"

namespace NSQLComplete {

    namespace {

        class TNameService: public INameService {
        public:
            explicit TNameService(ISchemaGateway::TPtr schema)
                : Schema_(std::move(schema))
            {
            }

            TFuture<TNameResponse> Lookup(TNameRequest request) const override {
                TListRequest list;
                list.Cluster = "";
                list.Path = request.Prefix;
                list.Filter.Types = THashSet<TString>();
                if (request.Constraints.Folder) {
                    list.Filter.Types->emplace(TFolderEntry::Folder);
                }
                if (request.Constraints.Table) {
                    list.Filter.Types->emplace(TFolderEntry::Table);
                }
                list.Limit = request.Limit;

                return Schema_->List(std::move(list)).Apply([](auto f) {
                    TListResponse list = f.ExtractValue();

                    TNameResponse response;
                    for (auto& entry : list.Entries) {
                        TGenericName name;
                        if (entry.Type == TFolderEntry::Folder) {
                            TFolderName local;
                            local.Indentifier = std::move(entry.Name);
                            name = std::move(local);
                        } else if (entry.Type == TFolderEntry::Table) {
                            TTableName local;
                            local.Indentifier = std::move(entry.Name);
                            name = std::move(local);
                        } else {
                            TUnkownName local;
                            local.Content = std::move(entry.Name);
                            local.Type = std::move(entry.Type);
                            name = std::move(local);
                        }
                        response.RankedNames.emplace_back(std::move(name));
                    }
                    return response;
                });
            }

        private:
            ISchemaGateway::TPtr Schema_;
        };

    } // namespace

    INameService::TPtr MakeSchemaNameService(ISchemaGateway::TPtr schema) {
        return INameService::TPtr(new TNameService(std::move(schema)));
    }

} // namespace NSQLComplete
