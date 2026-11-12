// Maintained Markdown navigation guard. Supplied spec parts are source material:
// their example links are not repository navigation and are deliberately excluded.
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;
std::string read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer; buffer << file.rdbuf(); return buffer.str();
}
std::string prose(const std::string& text) {
    std::istringstream input(text); std::string line, result; char fence = 0;
    while (std::getline(input, line)) {
        const auto first = line.find_first_not_of(" \t\r");
        const auto start = first == std::string::npos ? std::string{} : line.substr(first);
        if (start.starts_with("```") || start.starts_with("~~~")) {
            if (!fence) fence = start[0]; else if (fence == start[0]) fence = 0;
            result += '\n'; continue;
        }
        result += fence ? "\n" : line + '\n';
    }
    return result;
}
std::string decode(const std::string& value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size() &&
            std::isxdigit(static_cast<unsigned char>(value[i+1])) &&
            std::isxdigit(static_cast<unsigned char>(value[i+2]))) {
            out += static_cast<char>(std::stoi(value.substr(i+1,2), nullptr, 16)); i += 2;
        } else out += value[i];
    }
    return out;
}
std::string slug(std::string heading) {
    static const std::regex linkText(R"(!?\[([^\]]+)\]\([^)]*\))"), tag("<[^>]*>");
    heading = std::regex_replace(heading, linkText, "$1");
    heading = std::regex_replace(heading, tag, "");
    std::string result;
    for (std::size_t i=0; i<heading.size(); ++i) {
        const auto c=static_cast<unsigned char>(heading[i]);
        if (c<128) {
            if (std::isalnum(c) || c=='_' || c=='-') result += static_cast<char>(std::tolower(c));
            else if (c==' ') result += '-';
        } else {
            const auto length=(c&0xe0)==0xc0 ? 2u : (c&0xf0)==0xe0 ? 3u : 4u;
            const auto token=heading.substr(i,length);
            // Markdown headings here use UTF-8 en/em dashes and typographic punctuation.
            // Keep non-ASCII words; GitHub removes these punctuation/arrow/emoji blocks.
            const auto next=i+1<heading.size() ? static_cast<unsigned char>(heading[i+1]) : 0u;
            if (!(c==0xe2 && (next==0x80 || next==0x81 || next==0x86 || next==0x87)) && c!=0xf0)
                result += token;
            i += length-1;
        }
    }
    return result;
}
std::set<std::string> anchors(const std::string& text) {
    std::set<std::string> result; std::map<std::string, unsigned> seen;
    const auto body=prose(text);
    static const std::regex heading(R"(^ {0,3}#{1,6}\s+(.+?)\s*#*\s*$)");
    std::istringstream input(body); std::string line; std::smatch match;
    while (std::getline(input,line)) if (std::regex_match(line,match,heading)) {
        const auto name=slug(match[1]); const auto count=seen[name]++;
        result.insert(name+(count ? "-"+std::to_string(count) : ""));
    }
    static const std::regex explicitAnchor(R"anchor(<a\s+(?:[^>]*\s)?(?:id|name)=["']([^"']+))anchor");
    for (std::sregex_iterator it(body.begin(),body.end(),explicitAnchor), end; it!=end; ++it)
        result.insert((*it)[1]);
    return result;
}
std::vector<std::string> links(const std::string& text) {
    const auto body=prose(text); std::vector<std::string> result;
    static const std::regex link(R"(\]\((<?[^\s)]+>?))");
    for (std::sregex_iterator it(body.begin(),body.end(),link), end; it!=end; ++it) {
        auto url=(*it)[1].str();
        if (url.starts_with('<') && url.ends_with('>')) url=url.substr(1,url.size()-2);
        result.push_back(url);
    }
    return result;
}
bool external(const std::string& url) {
    static const std::regex scheme(R"(^[A-Za-z][A-Za-z0-9+.-]*:)");
    return url.starts_with("//") || std::regex_search(url,scheme);
}
bool sourcePart(const fs::path& path) {
    const auto name=path.generic_string();
    return name.starts_with("docs/specs/") && path.filename().string().starts_with("part-");
}
// In-memory existence uses the same relative resolution as a checkout, for negative fixtures.
using Documents = std::map<std::string, std::string>;
std::string normalized(const fs::path& path) {
    auto name=path.lexically_normal().generic_string();
    while (name.size()>1 && name.ends_with('/')) name.pop_back();
    return name;
}
// Facts a document states that the repository can answer (D502). `latestSchema` 0 skips its check.
// - A decision id (D12, D490a) names a row of a decision record: an `id="d12"` anchor under docs/decisions/.
// - No document cites a project schema above the reader's kLatestSchema.
// - data/projects/README.md names every shipped project, and no project that is not there.
void checkFacts(const Documents& docs, const std::set<std::string>& files, int latestSchema, std::vector<std::string>& errors) {
    std::set<std::string> decisions;
    const std::regex anchor("id=\"(d[0-9]+[a-z]?)\""), id("(^|[^A-Za-z0-9_-])D([1-9][0-9]{0,2}[a-z]?)(?![A-Za-z0-9])"),
        schema("[Ss]chemas?\\s+([0-9]{1,3})(\\s*(–|-|to)\\s*([0-9]{1,3}))?");
    for (const auto& [path,text]:docs)
        if (path.starts_with("docs/decisions/"))
            for (std::sregex_iterator it(text.begin(),text.end(),anchor),end; it!=end; ++it) decisions.insert((*it)[1]);
    for (const auto& [path,text]:docs) {
        if (sourcePart(path)) continue;
        const auto body=prose(text);
        std::set<std::string> reported;
        if (!decisions.empty())
            for (std::sregex_iterator it(body.begin(),body.end(),id),end; it!=end; ++it) {
                const auto name=(*it)[2].str();
                // D18 names the family D18a-D18d.
                if (!decisions.contains("d"+name) && !decisions.contains("d"+name+"a") && reported.insert(name).second) errors.push_back(path+": unknown decision D"+name);
            }
        if (latestSchema>0)
            for (std::sregex_iterator it(body.begin(),body.end(),schema),end; it!=end; ++it)
                for (const auto group:{1,4})
                    if ((*it)[group].matched && std::stoi((*it)[group])>latestSchema && reported.insert("s"+(*it)[group].str()).second)
                        errors.push_back(path+": schema "+(*it)[group].str()+" is above kLatestSchema "+std::to_string(latestSchema));
    }
    const std::string index="data/projects/README.md", folder="data/projects/", suffix=".traffic.json";
    if (!docs.contains(index)) return;
    for (const auto& file:files)
        if (file.starts_with(folder) && file.ends_with(suffix) && docs.at(index).find("`"+file.substr(folder.size())+"`")==std::string::npos)
            errors.push_back(index+": shipped project not listed: "+file);
    const std::regex row("(^|\n)\\| `([^`]+\\.traffic\\.json)`");
    for (std::sregex_iterator it(docs.at(index).begin(),docs.at(index).end(),row),end; it!=end; ++it)
        if (!files.contains(folder+(*it)[2].str())) errors.push_back(index+": lists a project that is not shipped: "+(*it)[2].str());
}
std::vector<std::string> check(const Documents& docs, const std::set<std::string>& files, int latestSchema=0) {
    std::vector<std::string> errors;
    checkFacts(docs,files,latestSchema,errors);
    // One parse per target document: RECORD.md alone is the target of ~330 anchored links.
    std::map<std::string, std::set<std::string>> anchorsOf;
    for (const auto& [path,text]:docs) {
        if (sourcePart(path)) continue;
        for (auto url:links(text)) {
            if (external(url)) continue;
            url=decode(url); const auto hash=url.find('#');
            const auto target=url.substr(0,hash);
            const auto resolved=target.empty() ? path :
                normalized(fs::path(path).parent_path()/target);
            if (!files.contains(resolved)) { errors.push_back(path+": missing "+url); continue; }
            if (hash==std::string::npos || hash+1>=url.size() || !docs.contains(resolved)) continue;
            auto found=anchorsOf.find(resolved);
            if (found==anchorsOf.end()) found=anchorsOf.emplace(resolved,anchors(docs.at(resolved))).first;
            if (!found->second.contains(url.substr(hash+1))) errors.push_back(path+": missing anchor "+url);
        }
    }
    // Each maintained document has one folder index. Archive/evidence/specs have their own conventions.
    for (const auto& folder:{"docs","docs/reference","docs/plans","docs/audits","docs/decisions"}) {
        const auto index=std::string(folder)+"/README.md";
        if (!docs.contains(index)) { errors.push_back("Missing index: "+index); continue; }
        std::set<std::string> listed;
        for (const auto& url:links(docs.at(index))) {
            if (!external(url)) listed.insert(normalized(fs::path(folder)/decode(url.substr(0,url.find('#')))));
        }
        for (const auto& [path,text]:docs) {
            (void)text;
            if (fs::path(path).parent_path().generic_string()==folder && path!=index && !listed.contains(path))
                errors.push_back(index+": unindexed "+path);
        }
    }
    // Each source module's README is its map: every header starts a table row there, as `name.*` or
    // `name.hpp`; a prose mention does not count. A folder without a README is mapped by its
    // nearest parent's.
    for (const auto& file:files) {
        if (!file.starts_with("src/") || !file.ends_with(".hpp")) continue;
        auto folder=fs::path(file).parent_path();
        while (folder.has_parent_path() && folder!="src" && !docs.contains((folder/"README.md").generic_string()))
            folder=folder.parent_path();
        const auto index=(folder/"README.md").generic_string();
        if (folder=="src" || !docs.contains(index)) { errors.push_back("Missing module map for "+file); continue; }
        if (("\n"+docs.at(index)).find("\n| `"+fs::path(file).stem().string()+".")==std::string::npos)
            errors.push_back(index+": unmapped "+file);
    }
    return errors;
}
int selfTest() {
    Documents docs{{"README.md","[Guide](docs/reference/Guide.md#same-1)\n[Decision](docs/decisions/RECORD.md#d1)\n"},
        {"docs/README.md","# Docs\n"},{"docs/reference/README.md","[Guide](Guide.md)\n"},
        {"docs/reference/Guide.md","# Same\n# Same\n```md\n[example](missing.md)\n```\n"},
        {"docs/plans/README.md","# Plans\n"},{"docs/audits/README.md","# Audits\n"},
        {"docs/decisions/README.md","[Record](RECORD.md)\n"},
        {"docs/decisions/RECORD.md","# Decisions\n<a id=\"d1\"></a>\n"},
        {"docs/specs/link/part-01.md","[source example](missing.md)\n"}};
    std::set<std::string> files; for (const auto& [path,text]:docs) { (void)text; files.insert(path); }
    if (!check(docs,files).empty()) return 1;
    docs["README.md"]+="[Broken](missing.md)\n";
    if (check(docs,files).empty()) return 1;
    docs["README.md"]="[Stale](docs/reference/Guide.md#absent)\n";
    if (check(docs,files).empty()) return 1;
    docs["README.md"]="[Encoded](docs/reference/Guide.md#same%2D1)\n";
    if (!check(docs,files).empty()) return 1;
    docs["docs/reference/README.md"]="# Unindexed\n";
    if (check(docs,files).empty()) return 1;
    docs["docs/reference/README.md"]="[Guide](Guide.md)\n";
    files.insert("src/core/types.hpp"); files.insert("src/model/network/network.hpp");
    if (check(docs,files).empty()) return 1; // no module map at all
    docs["src/core/README.md"]="| `types.hpp` |\n"; docs["src/model/README.md"]="| `other.*` |\n";
    files.insert("src/core/README.md"); files.insert("src/model/README.md");
    if (check(docs,files).empty()) return 1; // network.hpp unmapped in its parent's README
    docs["src/model/README.md"]="See `network.cpp`.\n";
    if (check(docs,files).empty()) return 1; // a prose mention is not a row
    docs["src/model/README.md"]="| `network.hpp` |\n";
    if (!check(docs,files).empty()) return 1;
    // Facts (D502): an unknown decision, a schema above the reader's, a project list out of step.
    docs["README.md"]="D1 and D1a stand.\n";
    if (check(docs,files).size()!=1) return 1; // D1a has no row; D1 has
    docs["README.md"]="GD7, D0 and a fenced example are not citations.\n```\nD9\n```\n";
    if (!check(docs,files).empty()) return 1;
    docs["README.md"]="Readers accept schemas 1–12; schema 13 adds nothing yet.\n";
    if (!check(docs,files,13).empty() || check(docs,files,12).size()!=1) return 1;
    docs["README.md"]="";
    docs["data/projects/README.md"]="| `a.traffic.json` | one |\n"; files.insert("data/projects/README.md");
    if (check(docs,files).size()!=1) return 1; // listed, not shipped
    files.insert("data/projects/a.traffic.json"); files.insert("data/projects/b.traffic.json");
    if (check(docs,files).size()!=1) return 1; // b shipped, not listed
    docs["data/projects/README.md"]+="| `b.traffic.json` | two |\n";
    if (!check(docs,files).empty()) return 1;
    std::cout << "Documentation positive/negative fixtures passed\n"; return 0;
}
}
int main(int argc,char** argv) {
    if (argc==2 && std::string(argv[1])=="--self-test") return selfTest();
    if (argc!=2) { std::cerr << "Usage: multra-check-docs <repository> | --self-test\n"; return 2; }
    const fs::path root=argv[1]; Documents docs; std::set<std::string> files;
    if (!fs::is_directory(root)) { std::cerr << "Not a repository directory: " << root << '\n'; return 2; }
    for (auto it=fs::recursive_directory_iterator(root); it!=fs::recursive_directory_iterator(); ++it) {
        const auto name=it->path().filename().string();
        if (it->is_directory()) {
            if (name.starts_with('.') || name.starts_with("build") || name=="node_modules" || name=="dist" || name=="out")
                it.disable_recursion_pending();
        }
        const auto path=it->path().lexically_relative(root).generic_string(); files.insert(path);
        if (it->is_regular_file() && it->path().extension()==".md") docs.emplace(path,read(it->path()));
    }
    // The reader's newest schema, from the one place the code states it.
    std::smatch latest; const auto schemaSource=read(root/"src/project/latest_schema.hpp");
    if (!std::regex_search(schemaSource,latest,std::regex("kLatestSchema\\s*=\\s*([0-9]+)"))) {
        std::cerr << "Cannot read kLatestSchema from src/project/latest_schema.hpp\n"; return 2;
    }
    const auto errors=check(docs,files,std::stoi(latest[1]));
    for (const auto& error:errors) std::cerr << error << '\n';
    if (errors.empty()) std::cout << "Documentation links, anchors, maintained indexes, module maps and stated facts passed\n";
    return errors.empty() ? 0 : 1;
}
