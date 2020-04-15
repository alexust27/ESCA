#ifndef FORMULASMT_H
#define FORMULASMT_H

#include "VersionedVariable.h"

class FormulaSMT
{
public:
    virtual std::string FormatSMTLIB() = 0;
};

#endif
