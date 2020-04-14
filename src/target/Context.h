#include "../SMT/Variable.h"
#include "../SMT/VersionedVariable.h"
#include "Module.h"

namespace Target
{
struct Context
{
    bool isIf = false;
    Statement **nextInIf = nullptr;

    Module module;

    std::vector<CompoundStatement *> stackSt;

    Function *lastFoo;

    //Statement* getLastPoped() {
    //    auto tmp = lastPoped;
    //    lastPoped = nullptr;
    //    return tmp;
    //}

    void setIf( Statement *s )
    {
        if( nextInIf )
        {
            *nextInIf = s;
            nextInIf = nullptr;
        }
    }

    void startIfSt( Statement **s )
    {
        nextInIf = s;
        isIf = true;
    }

    //Statement* getAfterIfStatement() {
    //    auto tmp = nextInIf;
    //    nextInIf = nullptr;
    //    isIf = false;
    //    return tmp;
    //}

    void addFoo()
    {
        lastFoo = module.addFoo();
//            lastFoo = Function();
    }

    void addToLast( Statement *s )
    {
        //if (isIf) {
        //    isIf = false;
        //    nextInIf = s;
        //}
        stackSt.back()->addState(s);
    }

    Statement *addDeleteStatement( const std::string &name, bool isArray )
    {
        auto s = new DeleteStatement(name, isArray);
        addToLast(s);
        return s;
    }

    Statement *
    addIfStatement( Statement *thenSt, Statement *elseSt, const std::string &condStr, const std::string &elseStr )
    {
        auto s = new IfStatement(thenSt, elseSt, condStr, elseStr);
        addToLast(s);
        return s;
    }

    //    просто создаем набор стейтов и добавляем его на вершину стэка,
    // не пуша никуда, для пуша в контекст есть отдельный метод
    Statement *createCompoundStatement( bool addToStates = true )
    {
        //addToStates = true;
        auto s = new CompoundStatement();
        // начало функции
        if( stackSt.empty())
        {
            lastFoo->statement = s;
        }
        //lastPoped = nullptr;
        if( !stackSt.empty() && addToStates )
        {
            addToLast(s);
        }
        stackSt.push_back(s);
        return s;
    }

    //кладет с вершины стэка в общую схему
    // FIXME: видимо больше не кладет
    void addCompoundStatement()
    {
        //if (stackSt.size() > 1) {
        //    stackSt[stackSt.size() - 2]->addState(stackSt.back());
        //}
        //addToLast(stackSt.back());
    }

    Statement *addVarDeclFromFoo( const std::string &varName, const std::string &fooName, const std::string &loc )
    {
        auto s = new VarDeclFromFooStatement(varName, fooName, loc);
        addToLast(s);
        return s;
    }

    Statement *addVarDeclNew( const std::string &varName, bool isArray, const std::string &loc )
    {
        auto s = new VarDeclNewStatement(varName, isArray, loc);
        addToLast(s);
        return s;
    }

    Statement *
    addVarAssigmentFromFoo( const std::string &varName, const std::string &fooName, const std::string &loc )
    {
        auto s = new VarAssigmentFromFooStatement(varName, fooName, loc);
        addToLast(s);
        return s;
    }

    Statement *
    addVarAssigmentFromPointer( const std::string &varName, const std::string &rhsName, const std::string &loc )
    {
        auto s = new VarAssigmentFromPointerStatement(varName, rhsName, loc);
        addToLast(s);
        return s;
    }

    Statement *addVarAssigmentNew( const std::string &varName, bool isArray, const std::string &loc )
    {
        auto s = new VarAssigmentNewStatement(varName, isArray, loc);
        addToLast(s);
        return s;
    }

    Statement *addReturn( const std::string &returnVarName )
    {
        auto s = new ReturnStatement(returnVarName);
        addToLast(s);
        return s;
    }

    void popCompound()
    {
        //lastPoped = stackSt.back();
        stackSt.pop_back();
    }

};
}
