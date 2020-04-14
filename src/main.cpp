#include <iostream>
#include <string>
#include <filesystem>

#include "AST/ASTWalker.h"
#include "target/Function.h"

extern std::map<std::string, std::string> staticFuncMapping;

int main( int argc, char **argv )
{
    std::string resource_path = "/home/alex/CLionProjects/ESCA/resource/";
    std::string test_file = "1.cpp";

    //name = "rtk";
    //name = "Its";
    //name = "gnss";
    //name = "quake";
    //name = "facebook";
    //name = "arduino";
    //name = "stlink";

//    std::fstream fstream(root + name + ".txt", std::ios_base::in);
//    std::string file;
//    while (std::getline(fstream, file)) {
//        walker.WalkAST(root + file);
//        llvm::errs() << "Parsed " << ++count << " files\n";
//    }

    ASTWalker walker;
#ifdef __linux__
    std::vector<std::string> paths = {
            "/usr/include/",
            "/usr/include/c++/9/",
            "/usr/include/x86_64-linux-gnu/",
            "/usr/include/x86_64-linux-gnu/c++/7/",
            "/usr/lib/gcc/x86_64-linux-gnu/7/include/"
    };
    walker.SetIncludeDirectories(paths);
#endif

    std::cout << "Start walk" << std::endl;

    freopen("debug_info.txt", "w", stdout); // всякий мусор будем выводить в файл

    walker.WalkAST(resource_path + test_file);

    std::cout << "---------------------------------------" << std::endl;
//    allocatedFunctions.clear();
//    allocatedFunctions.insert(std::string("new"));

    for( auto p : allFunctions )
    {
        staticFuncMapping.clear();
        p.second->process();
    }

//    DefectStorage::Instance().SaveDefects();

    return 0;
}