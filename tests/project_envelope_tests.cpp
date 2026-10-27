#include "test.hpp"
#include "project_envelope_cases.hpp"
#include "../src/project/document_envelope.hpp"
#include <limits>

using namespace multra;
TEST(project, envelope_codec_preserves_presence_types_and_numeric_boundaries) {
    for(const auto& c:envelopeCases::cases()) {
        bool accepted=true;
        try {(void)parseDocument(c.document);}catch(const std::exception&) {accepted=false;}
        if(accepted!=c.codecAccepts)throw std::runtime_error("Unexpected envelope result: "+c.name);
    }
    // Missing schemaVersion deliberately selects the existing scenario path.
    auto legacy=documentJson(ProjectDocument{});legacy.erase("schemaVersion");
    CHECK(!legacy.contains("schemaVersion"));CHECK(parseDocument(legacy)==ProjectDocument{});
}
TEST(project, envelope_schema_declares_partial_coverage_and_exact_integer_limits) {
    const auto schema=projectEnvelopeSchema();
    CHECK(schema.at("x-multra-complete")==false);
    CHECK(schema.at("x-multra-coverage")=="envelope-background");
    CHECK(schema.at("additionalProperties")==true);
    const auto& fields=schema.at("properties");
    CHECK(fields.at("nextId").at("maximum")==std::numeric_limits<std::uint64_t>::max()-1);
    CHECK(fields.at("revision").at("minimum")==0);
    CHECK(fields.at("schemaVersion").at("maximum")==kLatestSchema);
    CHECK(fields.at("background").at("additionalProperties")==true);
    CHECK(!fields.at("network").contains("properties"));
    const auto serialized=schema.dump();
    CHECK(serialized.find("18446744073709551614")!=std::string::npos);
    CHECK(Json::parse(serialized)==schema);
}
TEST(project, envelope_typed_background_keeps_byte_and_finite_constraints) {
    ProjectDocument d;
    for(auto member:{&BackgroundImage::x,&BackgroundImage::y,&BackgroundImage::metresPerPixel,
                     &BackgroundImage::rotation,&BackgroundImage::opacity}) {
        for(const auto bad:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
            d.background=BackgroundImage{};d.background.*member=bad;
            CHECK(!std::isfinite(d.background.*member));
            test::throws([&]{validateDocument(d);},"EDIT_BACKGROUND_INVALID");
        }
    }
    d.background=BackgroundImage{};d.background.pngBase64.reset();
    CHECK(!d.background.pngBase64);
    test::throws([&]{validateDocument(d);},"EDIT_BACKGROUND_INVALID");
    constexpr std::size_t limit=32*1024*1024;
    const auto byteLimit=projectEnvelopeSchema().at("properties").at("background").at("properties").at("pngBase64").at("x-multra-maxUtf8Bytes");
    CHECK(byteLimit==limit);
    d.background.pngBase64=std::make_shared<const std::string>(limit,'x');validateDocument(d);
    // UTF-8 bytes, not character count: the extra character is two bytes.
    d.background.pngBase64=std::make_shared<const std::string>(std::string(limit-1,'x')+"é");
    CHECK(d.background.pngBase64->size()==limit+1);
    test::throws([&]{validateDocument(d);},"EDIT_BACKGROUND_INVALID");
}
TEST(project, envelope_error_codes_and_legacy_path_are_preserved) {
    const auto base=documentJson(ProjectDocument{});
    auto wrong=base;wrong["schemaVersion"]=nullptr;wrong.erase("nextId");
    CHECK(wrong.at("schemaVersion").is_null());
    test::throws([&]{parseDocument(wrong);},"EDIT_VERSION");
    wrong=base;wrong["nextId"]="bad";wrong["background"]=nullptr;
    CHECK(wrong.at("nextId").is_string());
    test::throws([&]{parseDocument(wrong);},"EDIT_ID_LIMIT");
    wrong=base;wrong["background"]["x"]=nullptr;
    CHECK(wrong.at("background").at("x").is_null());
    test::throws([&]{parseDocument(wrong);},"EDIT_BACKGROUND_INVALID");
}
