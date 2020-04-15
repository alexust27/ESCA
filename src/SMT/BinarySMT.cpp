#include <sstream>

#include "BinarySMT.h"

BinarySMT::BinarySMT(const VersionedVariable& l, const VersionedVariable& r, OperatorSMT o, bool n) 
    : FormulaSMT()
    , lhs(l)
    , rhs(r)
    , op(o)
    , negation(n)
{
}

std::string BinarySMT::FormatSMTLIB()
{
	std::string sop;
	switch (op)
	{
	case EqualSMT:
		sop = "= ";
		break;
	case LessSMT:
		sop = "< ";
		break;
	case GreaterSMT:
		sop = "> ";
		break;
	default:
		sop = "= ";
	}

	std::string negBeg;// =
	std::string negEnd;// = ")";
	if (negation)
	{
		negBeg = "not (";
		negEnd = ")";
	}

	std::stringstream ss;
	ss << "(assert( " << negBeg << sop << lhs.VersionedName() << " " << rhs.VersionedName() << ")" << negEnd << ")\n";
	return ss.str();
}
