/// @author alexust27
/// @date 20.04.2020.

#include <cassert>
#include <iostream>
#include "Context.h"


namespace Target
{

Context::Context()
{
    curFunction = nullptr;
    freeFunctions.insert("free");
    freeFunctions.insert("close");
    freeFunctions.insert("fclose");
#ifdef _WIN32
    freeFunctions.insert("closesoket");
#endif
}

Context &Context::Instance()
{
    static Context instance;
    return instance;
}

void Context::ResetFunction()
{
//    curFunction = nullptr;
    if( !compoundStatementsStack.empty())
    {
        std::cerr << "Something was wrong. Compound stack must be empty!" << std::endl;
        compoundStatementsStack.clear();
    }
    exceptionName.clear();
}


void Context::AddFunction( const std::string &name )
{
    assert(compoundStatementsStack.empty());
    assert(exceptionName.empty());
    auto funName = name;
//    if( allFunctions.find(funName) != allFunctions.end())
//    {
//        static int fNum = 1;
//        funName += std::to_string(fNum);
//        ++fNum;
//    }
    curFunction = new Function(funName);
    // тут могут возникать коллизии если у функций одинаковые названия и они в разных файлах
    allFunctions[ funName ] = curFunction;
}


void Context::AddToLast( Statement *s )
{
    assert(!compoundStatementsStack.empty());
    compoundStatementsStack.back()->AddState(s);
}

CompoundStatement *Context::CreateCompoundStatement()
{
    auto s = new CompoundStatement();
    if( compoundStatementsStack.empty())
    {
        // начало функции
        curFunction->MakeStart(s);
    }
    else
    {
        AddToLast(s);
    }
    compoundStatementsStack.push_back(s);
    return s;
}


void Context::PopCompound()
{
    compoundStatementsStack.pop_back();
}


void Context::AddFreeFunction( const std::string &function )
{
    freeFunctions.insert(function);
}

bool Context::IsFreeFunction( const std::string &function )
{
    return freeFunctions.count(function);
}

bool Context::CreateIfStatement( bool hasElse, const std::string &cond, const std::string &elseCond )
{
    if( compoundStatementsStack.size() > 4 )
    {
        return false;
    }
    auto thenSt = new CompoundStatement();
    auto elseSt = hasElse ? new CompoundStatement() : nullptr;

    auto op = compoundStatementsStack.back()->GetOptions();
    op.isThen = true;
    op.isElse = false;
    thenSt->SetOptions(op);

    auto ifSt = new IfStatement(thenSt, elseSt, cond, elseCond);
    AddToLast(ifSt);
    compoundStatementsStack.push_back(thenSt);
    return true;
}

void Context::SwitchToElse()
{
    compoundStatementsStack.pop_back(); // убираем then
    auto ifStmt = dynamic_cast<IfStatement *>(compoundStatementsStack.back()->GetStates().back());
    assert(ifStmt); // последний положенный должен быть if
    if( ifStmt->elseSt && exceptionName.empty())
    {
        auto op = compoundStatementsStack.back()->GetOptions();
        op.isThen = false;
        op.isElse = true;
        ifStmt->elseSt->SetOptions(op);
        compoundStatementsStack.push_back(ifStmt->elseSt);
    }
}

void Context::CreateThrow( const std::string &eName )
{
    exceptionName = eName;
}

bool Context::CreateTryStatement()
{
    if( compoundStatementsStack.size() > 4 )
    {
        return false;
    }
    auto tryBlock = new CompoundStatement();
    auto catchBlock = new CompoundStatement();
    auto trySt = new TryStatement(tryBlock, catchBlock);
    auto op = compoundStatementsStack.back()->GetOptions();
    op.isTry = true;
    tryBlock->SetOptions(op);
    AddToLast(trySt);
    compoundStatementsStack.push_back(tryBlock);
    return true;
}

void Context::CreateCatchStatement()
{
    compoundStatementsStack.pop_back(); // убираем блок try
    auto tryStmt = dynamic_cast<TryStatement *>(compoundStatementsStack.back()->GetStates().back());
    assert(tryStmt); // последний положенный должен быть try
    auto op = compoundStatementsStack.back()->GetOptions();
    op.isTry = false;
    op.isCatch = true;
    tryStmt->catchSt->SetOptions(op);
    compoundStatementsStack.push_back(tryStmt->catchSt);
}

Context::~Context()
{
    for( auto& f : allFunctions )
    {
        delete f.second;
    }
    allFunctions.clear();
}

}
