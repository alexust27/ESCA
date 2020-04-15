#ifndef ESCAASTVisitor_H
#define ESCAASTVisitor_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>

#include "PathStorage.h"
#include "../file.h"


class ESCAASTVisitor : public clang::RecursiveASTVisitor<ESCAASTVisitor>
{
public:
    ESCAASTVisitor();

    bool VisitFunctionDecl( clang::FunctionDecl *f );

public:

    inline void SetPath( const std::string &_path )
    {
        path->SetPath(_path);
    }

private:
    std::string getLocation( const clang::Stmt *st );

    void Reset();

    bool ProcessFunction( clang::FunctionDecl *f );

    bool ProcessStmt( clang::Stmt *stmt, bool = true );

    bool ProcessCompound( clang::CompoundStmt *body, bool );

    bool ProcessAssignment( clang::BinaryOperator *binop );

    bool ProcessDeclaration( clang::VarDecl *vd );

    bool ProcessDelete( clang::CXXDeleteExpr *del );

    bool ProcessReturn( clang::ReturnStmt *ret );

    bool ProcessIf( clang::IfStmt *ifstmt );

    bool ProcessReturnNone(); //Pointers are not returned.

    bool ProcessReturnPtr( clang::ReturnStmt *ret ); //Pointers are returned.
private:
    Target::Context ctx;
    clang::SourceManager *currSM = nullptr;

    std::shared_ptr<PathStorage> path;

    //std::map<std::string, std::vector<VersionedVariable> > variables;
    //std::map<std::string, int> variables;

    // ������ �������
    std::map<std::string, PtrCounter> variables;

    std::vector<VersionedVariable> allocated;
    FSM fsm;

    FairLeafPredicate fairPred;
    BranchLeafPredicate branchPred;

    clang::Expr *returnExpr = nullptr;
    std::string returnVarName;
};

#endif
