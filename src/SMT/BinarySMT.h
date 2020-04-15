#ifndef EQUALSMT_H
#define EQUALSMT_H

#include "FormulaSMT.h"

enum OperatorSMT
{
    EqualSMT,
    LessSMT,
    GreaterSMT
};

class BinarySMT : public FormulaSMT
{
public:
//	BinarySMT() : FormulaSMT() {}
    BinarySMT( const VersionedVariable &l, const VersionedVariable &r, OperatorSMT o, bool n );

    std::string FormatSMTLIB() override;

    inline void Lhs( const VersionedVariable &l )
    {
        lhs = l;
    }

    inline VersionedVariable Lhs() const
    {
        return lhs;
    }

    inline void Rhs( const VersionedVariable &r )
    {
        rhs = r;
    }

    inline VersionedVariable Rhs() const
    {
        return rhs;
    }

    inline void Op( OperatorSMT o )
    {
        op = o;
    }

    inline OperatorSMT Op() const
    {
        return op;
    }

    inline void Negation( bool n )
    {
        negation = n;
    }

    inline bool Negation() const
    {
        return negation;
    }

private:
    VersionedVariable lhs;
    VersionedVariable rhs;
    OperatorSMT op;
    bool negation;
};

#endif
