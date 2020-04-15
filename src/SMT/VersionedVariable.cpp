#include "VersionedVariable.h"

bool operator ==(const VersionedVariable& lhs, const VersionedVariable& rhs)
{
	return lhs.VersionedName() == rhs.VersionedName();
}

bool operator <(const VersionedVariable& lhs, const VersionedVariable& rhs)
{
	return lhs.VersionedName() < rhs.VersionedName();
}
