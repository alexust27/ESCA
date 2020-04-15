#ifndef STATEFSM_H
#define STATEFSM_H

#include <vector>
#include <deque>

#include "TypesFSM.h"
#include "../SMT/FormulaSMT.h"

typedef std::vector<VersionedVariable> VarStorage;
typedef std::deque<std::shared_ptr<FormulaSMT> > FormulaStorage;
typedef std::string ConditionEvent;

//class TransitionFSM;
std::string FormulaeToString( const std::deque<std::shared_ptr<FormulaSMT>> &formulae );

std::string FormulaeToStringSat( const std::deque<std::shared_ptr<FormulaSMT>> &formulae );

class StateFSM
{
public:
    FSMID id = -1;
    std::vector<FSMID> incoming;
    std::vector<FSMID> outgoing;

    VarStorage allocPointers;
    VarStorage allocArrays;
    VarStorage delPointers;
    VarStorage delArrays;
    FormulaStorage formulae;
    bool isEnd = false;
    bool isBranchLeaf = false; //Former leaf in which there is

    std::string PrintFormulae();

    std::string PrintFormulaeSat();

    [[nodiscard]] inline bool IsLeaf() const
    {
        return (!isEnd) && outgoing.empty();
    }
};

bool operator==( const StateFSM &lhs, const StateFSM &rhs );

bool operator<( const StateFSM &lhs, const StateFSM &rhs );

//typedef shared_ptr<StateFSM> StateFSMPtr;

#endif
