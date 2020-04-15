#include "VariableSMT.h"

#include <sstream>

using namespace std;

std::string VariableSMT::FormatSMTLIB()
{
	stringstream ss;
	ss << "(declare-const " << var->VersionedName() << " " << var->TypeSMT() << ")\n";
	return ss.str();
}
