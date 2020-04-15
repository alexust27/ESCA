#ifndef PATH_STORAGE_H
#define PATH_STORAGE_H

#include <string>

class PathStorage
{
public:
    PathStorage() = default;

    PathStorage( const std::string &path )
    {
        SetPath(path);
    }


    void SetPath( const std::string &path )
    {
        fullPath = path;
#ifdef __linux__
        auto pos = fullPath.find_last_of('/');
#endif
#ifdef _WIN32
        auto pos = fullPath.find_last_of("/\\");
#endif
        folder = fullPath.substr(0, pos);
        file = fullPath.substr(pos + 1);
    }


    std::string Path() const
    {
        return fullPath;
    }

    std::string File() const
    {
        return file;
    }

    std::string Folder() const
    {
        return folder;
    }

private:
    std::string fullPath;
    std::string file;
    std::string folder;
};

#endif