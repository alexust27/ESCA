/// @file AnalyzeProcess.cpp
///
/// @brief Класс для запуска анализатора на основе полученных состояний
///
/// @author alexust27
/// Contact: ustinov1998s@gmail.com
///

#include "AnalyzeProcess.h"

#include <VariableSMT.h>
#include <BinarySMT.h>
#include <iostream>

AnalyzeProcess::AnalyzeProcess()
{
    processContext = nullptr;
//    allocatedFunctions.insert(std::string("malloc"));
//    allocatedFunctions.insert(std::string("socket"));
//    allocatedFunctions.insert(std::string("accept"));
//    allocatedFunctions.insert(std::string("fopen"));
}

void AnalyzeProcess::StartAnalyze()
{
    std::cout << "Start analyze." << std::endl;
    for( const auto &f : Context::Instance().allFunctions )
    {
        ProcessFunction(f.second);
    }
//    Context::Instance()
    for( auto &f : Context::Instance().allFunctions )
    {
        delete f.second;
        f.second = nullptr;
    }

}

void AnalyzeProcess::ProcessFunction( Target::Function *function )
{
    auto funName = function->GetName();
    if( processedFunctions.find(funName) != processedFunctions.end())
    {
        // already in process
        return;
    }

    // process all callee
    for( const auto &c : function->callee )
    {
        auto calleeFun = Context::Instance().allFunctions.find(c);
        if( calleeFun == Context::Instance().allFunctions.end())
        {
            // unknown function
            continue;
        }

        // prevent recursion
        if( funName != calleeFun->first )
        {
            ProcessFunction(calleeFun->second);
        }
    }

    // process
    processContext = std::make_unique<ProcessContext>(funName);

    ProcessCompound(function->StartState());

    if( !function->returnName.empty())
    {
        processContext->fsm->SetReturnVarName(function->returnName);
    }
    processContext->fsm->ProcessReturnNone();
    processedFunctions.insert(funName);
    processContext.reset();
}

void AnalyzeProcess::ProcessStatement( Statement *stmt )
{
    switch( stmt->GetType())
    {
        case COMPOUND:
        {
            ProcessCompound(dynamic_cast<CompoundStatement *>(stmt));
            break;
        }
        case VarAssigmentNew:
        {
            ProcessVarAssigmentNew(dynamic_cast<VarAssigmentNewStatement *>(stmt));
            break;
        }
        case VarAssigmentFromFoo:
        {
            ProcessVarAssigmentFromFoo(dynamic_cast<VarAssigmentFromFooStatement *>(stmt));
            break;
        }
        case VarAssigmentFromPointer:
        {
            ProcessVarAssigmentFromPointer(dynamic_cast<VarAssigmentFromPointerStatement *>(stmt));
            break;
        }
        case DELETE:
        {
            ProcessDelete(dynamic_cast<DeleteStatement *>(stmt));
            break;
        }
        case IF:
        {
            ProcessIF(dynamic_cast<IfStatement *>(stmt));
            break;
        }
        case TRY:
        {
            ProcessCompound(dynamic_cast<TryStatement *>(stmt)->trySt);
            ProcessCompound(dynamic_cast<TryStatement *>(stmt)->catchSt);
            break;
        }
        case Return:
        {
            ProcessReturn(dynamic_cast<ReturnStatement *>(stmt));
            break;
        }
        default:
        {
            std::cerr << "Unknown Statement for analyze" << std::endl;
            return;
        }
    }
}


void AnalyzeProcess::ProcessCompound( Target::CompoundStatement *statement )
{
    if( !statement )
        return;
    for( auto st : statement->GetStates())
    {
        ProcessStatement(st);
    }
}

void AnalyzeProcess::ProcessVarAssigmentNew( VarAssigmentNewStatement *statement )
{
    auto varName = statement->varName;
    PtrCounter &lhsCnt = processContext->variables[ varName ];
    ++(lhsCnt.count);

    StateFSM state;
    VersionedVariable vv(varName, statement->loc, VAR_POINTER, lhsCnt.count);
    if( statement->isArray )
    {
        vv.MetaType(VAR_ARRAY_POINTER);
        lhsCnt.meta = VAR_ARRAY_POINTER;
        state.allocArrays.push_back(vv);
    }
    else
    {
        lhsCnt.meta = VAR_POINTER;
        state.allocPointers.push_back(vv);
    }
    //Отметить new.
    processContext->allocatedVars.push_back(vv);

    std::shared_ptr<VariableSMT> vvFormulae(new VariableSMT(vv));
    state.formulae.push_back(vvFormulae);

    processContext->fsm->AddStateToLeaves(state, processContext->fairPred);
}

void AnalyzeProcess::ProcessVarAssigmentFromFoo( VarAssigmentFromFooStatement *statement )
{
    PtrCounter &lhsCnt = processContext->variables[ statement->varName ];

    if( allocatedFunctions.find(statement->fooName) !=
        allocatedFunctions.end())
    {
        ++(lhsCnt.count);
        StateFSM state;
        VersionedVariable vv(statement->varName, statement->loc, VAR_POINTER, lhsCnt.count);
        {
            // TODO: поддержать как царь разные варианты для массива и просто указателя
            state.allocPointers.push_back(vv);
            lhsCnt.meta = VAR_POINTER;
        }
//        if( statement->isDecl )
//        {
        //Отметить new.
        processContext->allocatedVars.push_back(vv);
//        }

        std::shared_ptr<VariableSMT> vvForm(new VariableSMT(vv));
        state.formulae.push_back(vvForm);

        processContext->fsm->AddStateToLeaves(state, processContext->fairPred);
    }
}

void AnalyzeProcess::ProcessVarAssigmentFromPointer( VarAssigmentFromPointerStatement *statement )
{
    auto varName = statement->varName;
    PtrCounter &lhsCnt = processContext->variables[ varName ];
    int lhsVer = ++(lhsCnt.count);

    StateFSM state;
    VersionedVariable lhsVar(varName, statement->loc, VAR_POINTER, lhsVer);
    //VariableSMT *lhsForm = new VariableSMT();
    std::shared_ptr<VariableSMT> lhsForm(new VariableSMT(lhsVar));
    state.formulae.push_back(lhsForm);

    PtrCounter &rhsCnt = processContext->variables[ statement->rhsName ];
    int rhsVer = rhsCnt.count;
    VersionedVariable rhsVar(statement->rhsName, statement->loc, VAR_POINTER, rhsVer);
    //VariableSMT *rhsForm = new VariableSMT();
    std::shared_ptr<VariableSMT> rhsForm(new VariableSMT(rhsVar));
    state.formulae.push_back(rhsForm);
    lhsCnt.meta = rhsCnt.meta;

    //shared_ptr<VariableSMT> leftForm(new VariableSMT);
    //leftForm->Var(lhsVar);
    //state.formulae.push_back(leftForm);
    std::shared_ptr<FormulaSMT> bs(new BinarySMT(lhsVar, rhsVar, EqualSMT, false));
    state.formulae.push_back(bs);

    processContext->fsm->AddStateToLeaves(state, processContext->fairPred);
}


void AnalyzeProcess::ProcessDelete( DeleteStatement *statement )
{
    auto cntIter = processContext->variables.find(statement->name);
    VersionedVariable vv(statement->name, "unused", cntIter->second.meta, cntIter->second.count);

    processContext->fsm->AddDeleteState(vv, statement->isArray);
}

void AnalyzeProcess::ProcessIF( IfStatement *statement )
{
    // process then
    StateFSM s1;

    if( statement->thenSt )
    {
//        ctx.fsm->PushCondition(condStr);
        StateFSM s;
        processContext->fsm->AddStateToLeaves(s, processContext->fairPred, statement->condStr, false);
        ProcessStatement(statement->thenSt);
//        ctx.fsm->PopCondition();
    }

    // process else
    if( statement->elseSt )
    {
//        ctx.fsm->PushCondition(elseStr);
        StateFSM s;
        processContext->fsm->AddStateToLeaves(s, processContext->branchPred, statement->elseStr, false);
        ProcessStatement(statement->elseSt);
//        ctx.fsm->PopCondition();
    }
}


void AnalyzeProcess::ProcessReturn( ReturnStatement *statement )
{
    if( !statement->returnVarName.empty())
    {
        std::set<std::string> tmp;
        tmp.insert(statement->returnVarName);
        processContext->fsm->SetReturnVarName(tmp);
    }

    processContext->fsm->ProcessReturnNone();

    if( processContext->fsm->IsAllocReturns())
    {
        allocatedFunctions.insert(processContext->fsm->FunctionName());
    }

    processContext->fsm->ClearReturnVarName();
}



