#include "../src/project/network_schema.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string_view>

int main(int argc,char** argv) {
    if(argc==2 && std::string_view(argv[1])=="--help") {
        std::cout<<"Usage: multra-project-schema [--help]\n"
            "Print a partial draft-2020-12 envelope/background and Network/Link/Lane/Connector/reference schema as UTF-8 JSON.\n"
            "Nested fields, native integer tokens and UTF-8 byte limits still require the project codec.\n";
        return 0;
    }
    if(argc!=1) {std::cerr<<"Usage: multra-project-schema [--help]\n";return 2;}
    std::cout<<multra::projectDocumentSchema().dump(2)<<'\n';
    return std::cout?0:1;
}
