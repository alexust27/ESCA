#include <map>
#include <set>
#include <vector>

#include "ProcessCtx.h"

namespace Target
{
class Statement
{
public:
    virtual void process( ProcessCtx &ctx ) = 0;

    virtual ~Statement() = default;
};


class DeleteStatement : public Statement
{
public:
    DeleteStatement( const std::string &name, bool isArray );

    void process( Target::ProcessCtx &ctx );

private:
    std::string name;
    bool isArray;
};


class CompoundStatement : public Statement
{
public:
    CompoundStatement() = default;

    void addState( Target::Statement *st );

    void process( Target::ProcessCtx &ctx );

private:
    std::vector<Statement *> statements;
};


class IfStatement : public Statement
{
public:
    IfStatement( Target::Statement *thenSt, Target::Statement *elseSt, const std::string &condStr,
                 const std::string &elseStr );

    void process( Target::ProcessCtx &ctx );

private:
    Statement *thenSt = nullptr;
    Statement *elseSt = nullptr;
    std::string condStr;
    std::string elseStr;
};


class VarDeclFromFooStatement : public Statement
{
public:
    VarDeclFromFooStatement( const std::string &varName, const std::string &fooName,
                             const std::string &loc );

    void process( Target::ProcessCtx &ctx );

private:
    std::string varName;
    std::string fooName;
    std::string loc;
};


class VarDeclNewStatement : public Statement
{
public:
    VarDeclNewStatement( const std::string &varName, bool isArray, const std::string &loc );;

    void process( ProcessCtx &ctx ) override;

private:
    std::string varName;
    bool isArray;
    std::string loc;
};


class VarAssigmentFromFooStatement : public Statement
{
public:
    VarAssigmentFromFooStatement( const std::string &varName, const std::string &fooName, const std::string &loc );

    void process( ProcessCtx &ctx ) override;

private:
    std::string varName;
    std::string fooName;
    std::string loc;
};


class VarAssigmentFromPointerStatement : public Statement
{
public:
    VarAssigmentFromPointerStatement( const std::string &varName, const std::string &rhsName,
                                      const std::string &loc );

    void process( ProcessCtx &ctx ) override;

private:
    std::string varName;
    std::string rhsName;
    std::string loc;
};


class VarAssigmentNewStatement : public Statement
{
public:
    VarAssigmentNewStatement( const std::string &varName, bool isArray, const std::string &loc );

    void process( ProcessCtx &ctx ) override;

private:
    std::string varName;
    bool isArray;
    std::string loc;
};


class ReturnStatement : public Statement
{
public:
    ReturnStatement( const std::string &returnVarName );

    void process( ProcessCtx &ctx ) override;

private:
    std::string returnVarName;
};
}
