#ifndef TRANSITIONFSM_H
#define TRANSITIONFSM_H

#include "TypesFSM.h"

class TransitionFSM
{
public:
    FSMID id;
    FSMID start;
    FSMID end;

    std::string evt;

    bool operator==( const TransitionFSM &rhs )
    {
        return this->id == rhs.id;
    }

    bool operator<( const TransitionFSM &rhs )
    {
        return this->id < rhs.id;
    }

};

bool operator ==(const TransitionFSM& lhs, const TransitionFSM& rhs);
bool operator <(const TransitionFSM& lhs, const TransitionFSM& rhs);

#endif
