#include <iostream>
#include <filesystem>
#include <utils/DefectStorage.h>
#include "AST/ASTWalker.h"
#include "target/AnalyzeProcess.h"

#ifdef __unix__
const std::vector<std::string> INCLUDE_PATHS = {
        "/usr/include/",
        "/usr/include/c++/9/",
        "/usr/include/x86_64-linux-gnu/",
        "/usr/include/x86_64-linux-gnu/c++/9/",
        "/usr/lib/gcc/x86_64-linux-gnu/9/include/",
//        "/usr/lib/llvm-10/include/",
};
#endif

int main()
{
    const std::string RESOURCE_PATH = std::filesystem::current_path().string() + "/examples/";
    std::vector<std::string> files = {
//            "test1.cpp",
//            "test2.cpp",
//            "test3.cpp",
//            "test4.cpp",
            "test5.cpp",
    };

    ASTWalker walker(INCLUDE_PATHS);
    for( const auto &file: files )
    {
        if( !walker.WalkAST(RESOURCE_PATH + file))
        {
            std::cerr << "Failed to walk for file: " << file << std::endl;
        }
        std::cout << "Finish walk file: " << RESOURCE_PATH + file << std::endl;
    }

    AnalyzeProcess a;
    a.StartAnalyze();

    DefectStorage::Instance().PrintDefects();

    std::cout << "Working time: " << clock() / CLOCKS_PER_SEC << " sec" << std::endl;
    return 0;
}