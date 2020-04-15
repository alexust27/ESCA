#ifndef EXEC_SOLVER
#define EXEC_SOLVER


#ifdef __linux__
constexpr const char *z3solverCmdline = "/usr/bin/z3 ";
#endif
#ifdef _WIN32
static const char *z3solverCmdline = "C:\\z3.exe /smt2 ";
#endif

inline std::string runSolver( const std::string &file )
{
    std::string cmd = z3solverCmdline + file;
    FILE *pipe = popen(cmd.c_str(), "r");
    if( !pipe )
    {
        std::cerr << "Fail to open pipe" << std::endl;
        return "";
    }

    char buffer[128];
    std::string result;
    while( !feof(pipe))
    {
        if( fgets(buffer, 128, pipe) != nullptr )
        {
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}

#endif
