#include <stdexcept>
#include <memory>
#include <fstream>

#ifdef _WIN32
#include <filesystem>
#endif

#include <llvm/Support/raw_ostream.h>
#include <iostream>

#include "FSM.h"
#include "../SMT/BinarySMT.h"
#include "../utils/ExecSolver.h"
//#include "../utils/DefectStorage.h"
#include "../utils/Output.h"

std::string PrintSMT( int iSat, const FormulaStorage &f )
{
    std::stringstream ss;
#ifdef __linux__
    ss << "/tmp/";
#endif
#ifdef _WIN32
    auto tmp_path =  std::filesystem::temp_directory_path();
    ss << tmp_path.c_str() << "\\";
#endif
    ss << "form" << iSat << ".sat";
    std::ofstream outf(ss.str());
    outf << FormulaeToStringSat(f);
    outf.close();
    return ss.str();
}

FSM::FSM() : iSat(0)
{
    CreateStart();
}

void FSM::CreateStart()
{
    StateFSM start;
    start.id = 0;
    states.push_back(start);
}

void FSM::Reset()
{
#ifdef SAVEXML
    SaveToXML();
#endif
    StatesStorage().swap(states);
    TransitionsStorage().swap(transitions);
    events.clear();
    iSat = 0;
    std::string().swap(functionName);
    CreateStart();
}

bool FSM::GetState( FSMID id, StateFSM &s )
{
    int ss = states.size();
    if( id < ss && id >= 0 )
    {
        if( states[ id ].id == id )
        {
            s = states[ id ];
            return true;
        }
        else
        {
            std::stringstream sstr;
            sstr << "id == " << id << " and index in vector states must be the same";
            throw std::logic_error(sstr.str());
        }
    }
    return false;
}

template<class T>
bool MoveVector( std::vector<T> &source, std::vector<T> &dst )
{
    std::vector<T> tmp;
    tmp.swap(source);
    tmp.insert(tmp.end(), dst.begin(), dst.end());
    dst.swap(tmp);
    return true;
}

int FSM::StateToLeaf( int leafId, const StateFSM &newState )
{
    return StateToLeaf(leafId, newState, EPSILON);
}

int FSM::StateToLeaf( int leafId, const StateFSM &newState, const std::string &pred )
{
    StateFSM s = newState;
    TransitionFSM t;
    t.start = leafId;
    s.id = GetNewStateID();
    t.end = s.id;
    t.id = GetNewTransitionID();
    t.evt = pred;
    StateFSM &leaf = states[ leafId ];
    leaf.outgoing.push_back(t.id);
    s.incoming.push_back(t.id);
    //Move all the formulae from old leaf to new leaf.
    s.formulae.insert(s.formulae.begin(), leaf.formulae.begin(), leaf.formulae.end());
    leaf.formulae.clear();

    MoveVector(leaf.allocArrays, s.allocArrays);
    MoveVector(leaf.allocPointers, s.allocPointers);
    MoveVector(leaf.delArrays, s.delArrays);
    MoveVector(leaf.delPointers, s.delPointers);

    states.push_back(s);
    transitions.push_back(t);
    //states.insert(pair<FSMID, StateFSM>(s.id, s));
    //transitions.insert(pair<FSMID, TransitionFSM>(t.id, t));
    return s.id;
}


void FSM::AddStateToLeaves( const StateFSM &s, LeafPredicate &pred )
{
    AddStateToLeaves(s, pred, EPSILON, true);
}

//TODO: Refactor this function!
void FSM::AddStateToLeaves( const StateFSM &s, LeafPredicate &pred, const std::string &cond, bool finCond )
{
    size_t sz = states.size();
    for( int i = 0; i < sz; ++i )
    {
        //if (states[i].IsLeaf())
        if( pred(states[ i ]))
        {
            if( MatchEvents(i)) //This is an expensive check, so we want perform it when other conditions are checked.
            {
                states[ i ].isBranchLeaf = !finCond;
                // !!!WARNING !!! changed states size here
                StateToLeaf(i, s, cond);
            }
        }
    }
    /*
    for (StatesIter iter = states.begin(); iter != states.end(); ++iter)
    {
        if (iter->outgoing.empty()) //leaf
        {
            TransitionFSM t;
            t.start = iter->id;
            s.id = GetNewStateID();
            t.end = s.id;
            t.id = GetNewTransitionID();
            t.evt = TransitionFSM::EPSILON;
            states.insert(pair<FSMID, StateFSM>(s.id, s));
            transitions.insert(pair<FSMID, TransitionFSM>(t.id, t));
        }
    }
    */
}

/*
void FSM::AddBranchToLeaves(StateFSM s)
{
	for (StatesIter iter = states.begin(); iter != states.end(); ++iter)
	{
		if (iter->second.outgoing.empty()) //leaf
		{
			TransitionFSM t;
			t.start = iter->second.id;
			s.id = GetNewStateID();
			t.end = s.id;
			t.id = GetNewTransitionID();
			t.evt = TransitionFSM::EPSILON;
			states.insert(pair<FSMID, StateFSM>(s.id, s));
			transitions.insert(pair<FSMID, TransitionFSM>(t.id, t));
		}
	}
}
*/
void FSM::AddFormulaSMT( const std::shared_ptr<FormulaSMT> &f )
{
    int size = states.size();
    for( int i = 0; i < size; ++i )
    {
        if( states[ i ].outgoing.empty())
        {
            states[ i ].formulae.push_back(f);
        }
    }
}

void FSM::AddAllocPointer( const VersionedVariable &ap )
{
    int size = states.size();
    for( int i = 0; i < size; ++i )
    {
        if( states[ i ].outgoing.empty())
        {
            states[ i ].allocPointers.push_back(ap);
        }
    }
}

void FSM::HandleDeletePtr( const VersionedVariable &v, std::vector<VersionedVariable> &alloc,
                           std::vector<VersionedVariable> &del, StateFSM &delState )
{
    //write formulae.
    int size = alloc.size();
    //unsat => delete is correct
    FormulaStorage f = delState.formulae;
    for( int i = 0; i < size; ++i )
    {
        std::shared_ptr<BinarySMT> form(new BinarySMT(v, alloc[ i ], EqualSMT, true));
        f.push_back(form);
    }

    auto fileName = PrintSMT(iSat, f);

    auto solverResult = runSolver(fileName);

    if( solverResult.find("unsat") != -1 ) //unsat
    {
        Cout << "Correct delete\n";
        del.push_back(v);
    }
}

void FSM::AddDeleteState( const VersionedVariable &var, bool arrayForm )
{
    int size = states.size();
    for( int i = 0; i < size; ++i )
    {
        if( states[ i ].outgoing.empty())
        {
            StateFSM sdel;

            int sdelId = StateToLeaf(i, sdel);

            StateFSM &del = states[ sdelId ];
            const VersionedVariable &v(var);
            switch( var.MetaType())
            {
                case VAR_POINTER:
                    HandleDeletePtr(v, del.allocPointers, del.delPointers, del);
                    break;
                case VAR_ARRAY_POINTER:
                    HandleDeletePtr(v, del.allocArrays, del.delArrays, del);
                    break;
                default:
                    break;
            }
            ++iSat;
        }
    }
}

void FSM::ProcessReturnNone()
{
    //For each leaf
    for( StateFSM &s : states )
    {
        if( s.IsLeaf())
        {
            //Create a new end state
            CreateNewRetNoneState(s);
        }
    }
}

void FSM::SolveRet( bool isArray, const StateFSM &s )
{
    VarStorage alloc, del;
    //FormulaStorage f = s.formulae;
    if( isArray )
    {
        alloc = s.allocArrays;
        del = s.delArrays;
    }
    else
    {
        alloc = s.allocPointers;
        del = s.delPointers;
    }
    //for each allocated variable
    auto sAlloc = alloc.size();
    auto sDel = del.size();
    for( int i = 0; i < sAlloc; ++i )
    {
        auto name = alloc[ i ].Name();
        if( returnVarName.find(name) != returnVarName.end() && !isAllocReturns )
        {
            isAllocReturns = true;
            continue;
        }

        //check whether this variable is del list.
        FormulaStorage f = s.formulae; //TODO: do this more effective.
        for( int j = 0; j < sDel; ++j )
        {
            //write formulae.
            std::shared_ptr<BinarySMT> bs(new BinarySMT(alloc[ i ], del[ j ], EqualSMT, true)); //unsat => in del list.
            f.push_back(bs);
        }
        auto fileName = PrintSMT(iSat++, f);
        //if this variable is not in del list then there is a memory leak.
        //auto fileName = PrintSMT(iSat, s.formulae);

        auto solverResult = runSolver(fileName);

        if( solverResult.find("unsat") != -1 ) //unsat
        {
            std::cout << "~~~~OK~~~~" << std::endl;
            //No leak
        }
        else if( solverResult.find("sat") != -1 )
        {
            //Leak
            llvm::errs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~LEAK~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
            llvm::errs() << "Location: " << alloc[ i ].getLocation() << "\n";
            llvm::errs() << "variable name: " << alloc[ i ].Name() << "\n";

            //std::stringstream defect;
            //defect << "Memory leak. Variable name: " << alloc[i].Name();
            //DefectStorage::Instance().AddDefect(defect.str());
        }
    }

}

void FSM::CreateNewRetNoneState( StateFSM &leaf )
{
    StateFSM s;
    s.id = GetNewStateID();

    s.formulae.insert(s.formulae.begin(), leaf.formulae.begin(), leaf.formulae.end());
    leaf.formulae.clear();

    MoveVector(leaf.allocArrays, s.allocArrays);
    MoveVector(leaf.allocPointers, s.allocPointers);
    MoveVector(leaf.delArrays, s.delArrays);
    MoveVector(leaf.delPointers, s.delPointers);

    //Note:This may be parallelized
    SolveRet(true, s);
    SolveRet(false, s);

    //Add s to FSM.
    TransitionFSM tr;
    tr.id = GetNewTransitionID();
    tr.start = leaf.id;
    tr.end = s.id;
    leaf.outgoing.push_back(tr.id);
    transitions.push_back(tr);
    s.incoming.push_back(tr.id);
    s.isEnd = true;
    states.push_back(s);

}

bool FSM::MatchEvents( FSMID stateID )
{
    //ConditionStorage events = states[stateID].events;
    int cur = stateID;
    auto localEvents = events;
    while( !localEvents.empty())
    {
        if( states[ cur ].incoming.empty()) //start state.
        {
            break;
        }
        auto trID = states[ cur ].incoming[ 0 ]; //in the current model there is one parent of each state.
        TransitionFSM &tr = transitions[ trID ];
        auto parent = tr.start;
        if( !localEvents.empty())
        {
            if( tr.evt != EPSILON )
            {
                if( tr.evt != localEvents.back())
                {
                    return false;
                }
            }
        }
        cur = parent;
        localEvents.pop_back();
    }
    return true;
}

#ifdef DEBUG

namespace
{
int indent;

std::string DoIndent()
{
    std::string res = "";
    for( int i = 0; i < indent; ++i )
    {
        res.push_back('\t');
    }
    return res;
}

std::string CnvStateId( FSMID id )
{
    std::stringstream res;
    res << (id * 2);
    return res.str();
}

std::string CnvTransitionId( FSMID id )
{
    std::stringstream res;
    res << (id * 2 + 1);
    return res.str();
}
}

void FSM::SaveToXML()
{
    std::string path = functionName;
    if( path.empty())
    {
        path = "defaultFunction";
    }
    path += ".xstd";
    SaveToXML(path);
}

void FSM::SaveToXML( const std::string &path )
{
    std::ofstream outf(path);
    if( !outf.good())
    {
        return;
    }

    outf << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    outf << "<diagram>" << std::endl;

    ++indent;
    outf << DoIndent() << "<name>" << functionName << "</name>" << std::endl;

    outf << DoIndent() << "<data>" << std::endl;
    ++indent;
    outf << DoIndent() << "<Statemachine>" << std::endl;

    ++indent;
    for( auto &event : events )
    {
        outf << DoIndent() << "<event name=\"" << event << R"(" comment=""/>)" << std::endl;
    }
    outf << DoIndent() << "<autoreject>False</autoreject>" << std::endl;
    --indent;

    outf << DoIndent() << "</Statemachine>" << std::endl;
    --indent;
    outf << DoIndent() << "</data>" << std::endl;
    --indent;

    Cout << "States: ";

    for( auto &state : states )
    {
        outf << DoIndent() << "<widget id=\"" << CnvStateId(state.id) << "\" type=\"State\">" << std::endl;
        ++indent;
        outf << DoIndent() << "<attributes>" << std::endl;
        ++indent;
        outf << DoIndent() << "<name>state" << state.id << "</name>" << std::endl;
        outf << DoIndent() << "<type>";
        if( state.id )
        {
            //Cout << ((iter->isEnd) ? "0 " : "2 ");
            outf << ((!state.isEnd) ? "0 " : "2 ");
        }
        else
        {
            Cout << "1 ";

            outf << "1";
        }
        outf << "</type>" << std::endl;

        for( int &income : state.incoming )
        {
            outf << DoIndent() << "<incoming id=\"" << CnvTransitionId(income) << "\"/>" << std::endl;
        }

        for( int &outg : state.outgoing )
        {
            outf << DoIndent() << "<outgoing id=\"" << CnvTransitionId(outg) << "\"/>" << std::endl;
        }

        --indent;
        outf << DoIndent() << "</attributes>" << std::endl;
        --indent;
        outf << DoIndent() << "</widget>" << std::endl;
    }
    Cout << "\n";


    for( auto &transition : transitions )
    {
        outf << "<widget id=\"" << CnvTransitionId(transition.id) << "\" type=\"Transition\">" << std::endl;
        ++indent;
        outf << DoIndent() << "<attributes>" << std::endl;
        ++indent;
        outf << DoIndent() << "<event name=\"" << transition.evt << "\" comment=\"\" />" << std::endl;
        --indent;
        outf << DoIndent() << "</attributes>" << std::endl;
        --indent;
        outf << DoIndent() << "</widget>" << std::endl;
    }

    outf << "</diagram>" << std::endl;

    outf.close();
}

#endif