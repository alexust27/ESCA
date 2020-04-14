#include "../FSM/FSM.h"

namespace Target
{
struct ProcessCtx
{
    std::map <std::string, PtrCounter> variables;

    std::vector <VersionedVariable> allocated;
    FSM fsm;

    FairLeafPredicate fairPred;
    BranchLeafPredicate branchPred;
};
}