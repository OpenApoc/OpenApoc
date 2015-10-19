#include "game/organisation.h"
#include "library/strings.h"

namespace OpenApoc
{

Organisation::Organisation(const UString &ID, const UString &name, int balance, int income)
    : ID(ID), name(name), balance(balance), income(income)
{
}

bool Organisation::isHostileTo(const Organisation &other) const
{
	// FIXME: Everyone is hostile!
	return (this != &other);
}

}; // namespace OpenApoc
