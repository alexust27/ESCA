#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <filesystem>

#include "utils/common.h"
#include "AST/ASTWalker.h"
#include "target/AnalyzeProcess.h"
#include "utils/DefectStorage.h"


void usage()
{
    std::cout << "USAGE: ESCA [Options] <source file> " << std::endl
              << "\tOptions:\n\t[-f|--files] <text file with exec files>" << std::endl
              << "\t[-c|--cmake] <output file of compile commands>" << std::endl
              << "\t[--fast]  -  turn on fast analyze" << std::endl;
}

bool parseFile( std::vector<std::string> &files, const std::string &file, bool isCmake )
{
    if( isCmake && file.find(".json") == std::string::npos )
    {
        std::cerr << "Output file of compile commands file must be json" << std::endl;
        return false;
    }
    std::ifstream fs(file);
    if( !fs.is_open())
    {
        std::cerr << "Can't open file: " << file << std::endl;
        return false;
    }
    std::string line;
    while( getline(fs, line))
    {
        if( isCmake )
        {
            int s = line.find("\"file\"");
            if( s != std::string::npos )
            {
                line = line.substr(s + 9, line.length() - s - 10);
                if( std::filesystem::exists(line))
                {
                    files.push_back(line);
                }
            }
        }
        else
        {
            if( std::filesystem::exists(line))
            {
                files.push_back(line);
            }
        }
    }
    if( files.empty())
    {
        std::cerr << "Not found any file in " << file << std::endl;
        return false;
    }
    return true;
}

bool parseArgs( std::vector<std::string> &files, std::string &path, int argc, char **argv )
{
    if( argc < 2 )
    {
        std::cerr << "Not enough arguments" << std::endl;
        return false;
    }
    if( std::string(argv[ 1 ]) == "-h" || std::string(argv[ 1 ]) == "--help" )
    {
        return false;
    }
    auto flag = std::string(argv[ 1 ]);
    int curArgv = 2;
    if( flag == "--fast" )
    {
        if( argc == curArgv )
        {
            std::cerr << "Not enough arguments" << std::endl;
            return false;
        }
        ++curArgv;
        CommonStorage::Instance().needFast = true;
        flag = argv[ 2 ];
    }
    if( argc == curArgv )
    {
        if( std::filesystem::exists(flag))
        {
            files.emplace_back(flag);
            return true;
        }
        std::cerr << "File " << flag << " doesn't exist" << std::endl;
        return false;
    }
    assert(argc > curArgv);

    if( flag == "--files" || flag == "-f" )
    {
        return parseFile(files, argv[ curArgv ], false);
    }
    if( flag == "-c" || flag == "--cmake" )
    {
        return parseFile(files, argv[ curArgv ], true);
    }
    std::cerr << "Undefined args" << std::endl;
    return false;
}


int main( int argc, char **argv )
{
    std::vector<std::string> files;
    std::string path;
    if( !parseArgs(files, path, argc, argv))
    {
        usage();
        return 1;
    }
    if( CommonStorage::Instance().needFast )
    {
        std::cout << "fast mode ON" << std::endl;
    }

    std::vector<std::string> paths;
    paths = {
            "/usr/include/",
            "/usr/include/c++/9/",
            "/usr/include/x86_64-linux-gnu/",
            "/usr/include/x86_64-linux-gnu/c++/9/",
            "/usr/lib/gcc/x86_64-linux-gnu/9/include/",
//            "/usr/lib/llvm-10/include/",
    };
    ASTWalker walker(paths);


    std::cout << "Files for analyze: " << files.size() << std::endl;
    walker.WalkAST(files);

    AnalyzeProcess a;
    a.StartAnalyze();

    DefectStorage::Instance().PrintDefects("defects_output.txt");

    std::cout << "Working time: " << clock() / CLOCKS_PER_SEC << " sec" << std::endl;

    return 0;
}