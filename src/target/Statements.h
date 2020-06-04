/// @file Statements.h
///
/// @brief Класс всех состояний анализируемого кода
///
/// @author alexust27
/// Contact: ustinov1998s@gmail.com
///

#include <map>
#include <set>
#include <vector>


enum STATEMENTS
{
    COMPOUND,
    VarAssigmentNew,
    VarAssigmentFromFoo,
    VarAssigmentFromPointer,
    DELETE,
    Return,
    IF,
    TRY,
    UNKNOWN,
};

namespace Target
{
/// @brief Общий класс для всех состояний анализируемого кода
class Statement
{
public:

    virtual STATEMENTS GetType()
    {
        return STATEMENTS::UNKNOWN;
    }

    virtual ~Statement() = default;
};

/// @brief Составное состояние, содержит несколько состояний
class CompoundStatement : public Statement
{
public:
    CompoundStatement() = default;

    void AddState( Statement *st );

    const std::vector<Statement *> &GetStates() const;

    STATEMENTS GetType() override
    {
        return STATEMENTS::COMPOUND;
    }

    class IsInOptions
    {
    public:
        bool isThen = false;
        bool isElse = false;
        bool isTry = false;
        bool isCatch = false;
        bool isFor = false;
        bool isWhile = false;
    };

    void SetOptions( const CompoundStatement::IsInOptions &otherOptions );

    CompoundStatement::IsInOptions GetOptions() const;

    ~CompoundStatement() override
    {
        for( auto s : statements )
        {
            delete s;
        }
    }

private:

    IsInOptions op;
    std::vector<Statement *> statements;
};


class VarAssigmentFromFooStatement : public Statement
{
public:
    VarAssigmentFromFooStatement( const std::string &varName, const std::string &fooName, const std::string &loc,
                                  bool isDecl );

    STATEMENTS GetType() override
    {
        return STATEMENTS::VarAssigmentFromFoo;
    }

    bool isDecl;
    std::string varName;
    std::string fooName;
    std::string loc;
};

/// auto* varName, auto * rhsName;
/// varName = rhsName;
class VarAssigmentFromPointerStatement : public Statement
{
public:
    VarAssigmentFromPointerStatement( const std::string &varName, const std::string &rhsName,
                                      const std::string &loc, bool isDecl );

    STATEMENTS GetType() override
    {
        return STATEMENTS::VarAssigmentFromPointer;
    }

    std::string varName;
    std::string loc;
    std::string rhsName;
    bool isDecl;
};

/// x = new X;
class VarAssigmentNewStatement : public Statement
{
public:
    VarAssigmentNewStatement( const std::string &varName, bool isArray, const std::string &loc, bool isDecl );

    STATEMENTS GetType() override
    {
        return STATEMENTS::VarAssigmentNew;
    }

    std::string varName;
    std::string loc;
    bool isArray;
    bool isDecl;
};


class DeleteStatement : public Statement
{
public:
    DeleteStatement( const std::string &name, bool isArray );

    STATEMENTS GetType() override
    {
        return STATEMENTS::DELETE;
    }

    std::string name;
    bool isArray;
};


class ReturnStatement : public Statement
{
public:
    explicit ReturnStatement( const std::string &returnVarName );

    STATEMENTS GetType() override
    {
        return STATEMENTS::Return;
    }

    std::string returnVarName;
};

class IfStatement : public Statement
{
public:
    IfStatement( Target::CompoundStatement *thenSt, Target::CompoundStatement *elseSt, const std::string &condStr,
                 const std::string &elseStr );

    STATEMENTS GetType() override
    {
        return STATEMENTS::IF;
    }

    ~IfStatement() override
    {
        delete elseSt;
        delete thenSt;
    }

    CompoundStatement *thenSt = nullptr;
    CompoundStatement *elseSt = nullptr;
    std::string condStr;
    std::string elseStr;
};


class TryStatement : public Statement
{
public:
    TryStatement( CompoundStatement *trySt, CompoundStatement *catchSt ) : trySt(trySt), catchSt(catchSt)
    {
    }

    STATEMENTS GetType() override
    {
        return STATEMENTS::TRY;
    }

    ~TryStatement() override
    {
        delete trySt;
        delete catchSt;
    }


    CompoundStatement *trySt = nullptr;
    CompoundStatement *catchSt = nullptr;
};


} // target
