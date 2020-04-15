#ifndef VARIABLESMT_H
#define VARIABLESMT_H

#include <memory>

#include "FormulaSMT.h"

class VariableSMT : public FormulaSMT
{
public:
    std::string FormatSMTLIB() override;

    explicit VariableSMT( const VersionedVariable &v )
    {
        var = std::make_unique<VersionedVariable>(v);
    }

    inline VersionedVariable Var()
    {
        return *var;
    }


private:
    std::unique_ptr<VersionedVariable> var;
};

#endif
