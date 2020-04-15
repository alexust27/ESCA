#ifndef SMTSET_H
#define SMTSET_H

#include <set>

#include "VersionedVariable.h"

class SMTSet
{
public:
	inline void AddVar(VersionedVariable v) { vars.insert(v); }
private:
	std::set<VersionedVariable> vars;
};

#endif
