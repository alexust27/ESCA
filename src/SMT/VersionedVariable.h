#ifndef VERSIONED_VARIABLE_H
#define VERSIONED_VARIABLE_H

#include <sstream>

#include "Variable.h"

class VersionedVariable : public Variable
{
public:
    VersionedVariable() = delete;

    VersionedVariable( const VersionedVariable &vv ) = default;

    VersionedVariable( const std::string &name, const std::string &location, EMetaType mt, int ver, const std::string &type="" )
            : Variable(name, location, mt, type), version(ver)
    {
    }

    inline int Version() const
    {
        return version;
    }

    inline std::string VersionedName() const
    {
        return name + "!" + std::to_string(version);
    }

private:
    int version;
};

bool operator==( const VersionedVariable &lhs, const VersionedVariable &rhs );

bool operator<( const VersionedVariable &lhs, const VersionedVariable &rhs );

#endif
