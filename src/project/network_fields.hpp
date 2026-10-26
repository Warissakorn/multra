#pragma once
#include "json.hpp"
#include <span>

namespace multra::networkCodec {
enum class Object { network,link,lane,point };
enum class Kind { text,number,level,side,array,links,lanes,geometry,markings,unchecked };
struct Field {
    const char* name;Kind kind;bool required{};int knownFrom{};
    int rejectBefore{};bool nullable{};
};
inline constexpr int strictVersion=7,behaviourVersion=21,linkTypeVersion=50,roadVersion=53;
inline constexpr int levelLimit=1000;
std::span<const Field> fields(Object);
// Same legacy/additive known-key policy and diagnostics as parseNetwork.
void knownFields(const Json&,Object,const std::string& path,int version);
const Json& value(const Json&,Object,const char* name);
// Link-only decoder: leaves linkType/roadId to their existing document read stages.
std::vector<Link> readLinks(const Json& network,int version);
}
