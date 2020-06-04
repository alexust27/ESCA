#ifndef FSM_H
#define FSM_H

#include <set>

#include "TransitionFSM.h"
#include "LeafPredicate.h"

typedef std::vector<StateFSM>::iterator StatesIter;
typedef std::vector<TransitionFSM>::iterator TransitionsIter;
typedef std::deque<ConditionEvent> ConditionStorage;


class FSM
{
public:
    explicit FSM( const std::string &functionName );

    ~FSM();

    void AddStateToLeaves( const StateFSM &s, LeafPredicate &pred );

    void AddStateToLeaves( const StateFSM &s, LeafPredicate &pred, const std::string &cond, bool isBranchLeaf );

    void AddDeleteState( const VersionedVariable &var, bool arrayForm );

    void ProcessReturnNone();

    inline void SetReturnVarName( const std::set<std::string> &varName )
    {
        returnVarName = varName;
        isAllocReturns = false;
    }

    inline std::string FunctionName() const
    {
        return functionName;
    }

    inline void ClearReturnVarName()
    {
        returnVarName.clear();
        isAllocReturns = false;
    }

    /// @brief Возвращает ли функция указатель на выделенную память
    /// @return true - возвращает, иначе - false
    inline bool IsAllocReturns()
    {
        return isAllocReturns;
    }

#ifdef SAVE_XML

    void SaveToXML();

    void SaveToXML( const std::string &path );

#endif

    inline void AddState( const StateFSM &s )
    {
        states[ s.id ] = s;
    }

    inline void AddTransition( const TransitionFSM &trans )
    {
        transitions[ trans.id ] = trans;
    }

    bool GetState( FSMID id, StateFSM &s );

//    bool GetTransition( FSMID id, TransitionFSM &t );

//    inline void PushCondition( const std::string &cond )
//    {
//        events.push_back(cond);
//    }
//
//    inline void PopCondition()
//    {
//        if( !events.empty())
//            events.pop_back();
//    }


    //void AddBranchToLeaves(StateFSM s);
    void AddFormulaSMT( const std::shared_ptr<FormulaSMT> &f ); //Add SMT-formula to all leaves;
    void AddAllocPointer( const VersionedVariable &ap );

    //void AddTransition(shared_ptr<StateFSM> begin, shared_ptr<StateFSM> end, shared_ptr<Tra)

private:
//internal functions

    void CreateStart();

    inline FSMID GetNewStateID() const
    {
        return states.size();
    }

    inline FSMID GetNewTransitionID() const
    {
        return transitions.size();
    }

    int StateToLeaf( int leafId, const StateFSM &newState );

    /// Переносит всю информацию с последнего состояния на новое состояние
    int StateToLeaf( int leafId, const StateFSM &newState, const std::string &pred );

    void HandleDeletePtr( const VersionedVariable &v, std::vector<VersionedVariable> &alloc,
                          std::vector<VersionedVariable> &del, StateFSM &delState ) const;

    void CreateNewRetNoneState( StateFSM &leaf );

    void SolveRet( bool isArray, const StateFSM &s );

    bool MatchEvents( FSMID stateID );

private:
//member variables

    std::set<std::string> returnVarName;

    /// @brief флаг того что функция возвращает указатель на выделенную память
    bool isAllocReturns = false;

    std::string functionName;

    /// @brief Все вершины(состояния) автомата
    std::vector<StateFSM> states;

    /// @brief Все ребра (переходы) автомата
    std::vector<TransitionFSM> transitions;

    int iSat;

    const std::string EPSILON = "";

//    ConditionStorage events;
};

#endif
