#include <memory>
#include <string>

#include "StateFSM.h"

std::string FormulaeToString( const FormulaStorage &formulae )
{
    std::string res;
    for( auto &f : formulae )
    {
        res += f->FormatSMTLIB();
    }
    return res;
}

std::string FormulaeToStringSat( const FormulaStorage &formulae )
{
    return FormulaeToString(formulae) + "(check-sat)\n";
}


std::string StateFSM::PrintFormulae()
{
    return FormulaeToString(formulae);
}

std::string StateFSM::PrintFormulaeSat()
{
    return FormulaeToStringSat(formulae);
}


bool operator==( const StateFSM &lhs, const StateFSM &rhs )
{
    return lhs.id == rhs.id;
}

bool operator<( const StateFSM &lhs, const StateFSM &rhs )
{
    return lhs.id < rhs.id;
}
