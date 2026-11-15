#pragma once
// D574: the refusal checks the schema slices' tests share (D569–D573). Each parses a document
// through its saved text, as a file would be read.
#include "test.hpp"
#include "../src/core/validate.hpp"
#include "../src/project/document.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace schemaRefusal {
// The first issue is `code` at `path`.
inline void refusedAt(const multra::Json& document,const std::string& code,const std::string& path) {
    try {(void)multra::parseDocument(multra::Json::parse(document.dump()));}
    catch(const multra::ValidationError& e) {CHECK(e.issues[0].code==code);CHECK(e.issues[0].path==path);return;}
    throw std::runtime_error("Expected "+code+" at "+path);
}
using Fault=std::pair<std::string,std::string>;
// Exactly these issues, in this order.
inline void refusedWith(const multra::Json& document,const std::vector<Fault>& expected) {
    CHECK(!expected.empty());
    try {(void)multra::parseDocument(multra::Json::parse(document.dump()));}
    catch(const multra::ValidationError& e) {
        CHECK(e.issues.size()==expected.size());
        for(std::size_t i=0;i<expected.size() && i<e.issues.size();++i) {
            CHECK(e.issues[i].code==expected[i].first);CHECK(e.issues[i].path==expected[i].second);
        }
        return;
    }
    throw std::runtime_error("Expected "+expected[0].first+" at "+expected[0].second);
}
}
