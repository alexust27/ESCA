#ifndef LEAF_PREDICATE_H
#define LEAF_PREDICATE_H

#include "StateFSM.h"

class LeafPredicate
{
public:
    virtual bool operator()( const StateFSM &s ) = 0;
};

class FairLeafPredicate : public LeafPredicate
{
public:
    bool operator()( const StateFSM &s ) override
    {
        return (!s.isEnd) && s.outgoing.empty();
    }
};

class BranchLeafPredicate : public LeafPredicate
{
public:
    bool operator()( const StateFSM &s ) override
    {
        return (!s.isEnd) && s.isBranchLeaf;
    }
};

#endif
