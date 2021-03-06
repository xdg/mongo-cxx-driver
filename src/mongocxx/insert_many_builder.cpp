// Copyright 2015 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/concatenate.hpp>
#include <mongocxx/insert_many_builder.hpp>

#include <mongocxx/collection.hpp>

#include <mongocxx/config/private/prelude.hh>

namespace mongocxx {
MONGOCXX_INLINE_NAMESPACE_BEGIN

namespace {

options::bulk_write make_bulk_write_options(const options::insert& insert_options) {
    options::bulk_write bw;
    bw.ordered(insert_options.ordered().value_or(true));
    if (insert_options.write_concern()) {
        bw.write_concern(*insert_options.write_concern());
    }
    if (insert_options.bypass_document_validation()) {
        bw.bypass_document_validation(*insert_options.bypass_document_validation());
    }
    return bw;
}

}  // namespace

insert_many_builder::insert_many_builder(const options::insert& options)
    : _writes{make_bulk_write_options(options)}, _inserted_ids{} {}

void insert_many_builder::operator()(const bsoncxx::document::view& doc) {
    using bsoncxx::builder::basic::concatenate;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    bsoncxx::builder::basic::document id_doc;
    if (!doc["_id"]) {
        id_doc.append(kvp("_id", bsoncxx::oid{}));
        _writes.append(
            model::insert_one{make_document(concatenate(id_doc.view()), concatenate(doc))});
    } else {
        id_doc.append(kvp("_id", doc["_id"].get_value()));

        _writes.append(model::insert_one{doc});
    }
    _inserted_ids.append(id_doc.view());
};

stdx::optional<result::insert_many> insert_many_builder::insert(collection* col) const {
    auto result = col->bulk_write(_writes);
    if (!result) {
        return stdx::nullopt;
    }
    return stdx::optional<result::insert_many>{
        result::insert_many{std::move(result.value()), _inserted_ids.view()}};
};

MONGOCXX_INLINE_NAMESPACE_END
}  // namespace mongocxx
