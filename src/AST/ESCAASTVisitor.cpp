#include <string>
#include <map>
#include <vector>
#include <set>

#include <llvm/Support/raw_ostream.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/ASTContext.h>

#include "ESCAASTVisitor.h"

#include "PathStorage.h"
#include "Output.h"


std::map<std::string, std::string> staticFuncMapping;

std::set<std::string> allocatedFunctions;

std::map<std::string, Target::Function*> allFunctions;

std::set<std::string> processedFunctions;


std::string ESCAASTVisitor::getLocation( const clang::Stmt *st )
{
    auto loc = st->getBeginLoc();
    return loc.printToString(*currSM);
}


ESCAASTVisitor::ESCAASTVisitor() : path(new PathStorage)
{
}


bool ESCAASTVisitor::VisitFunctionDecl( clang::FunctionDecl *f )
{
    return ProcessFunction(f);
}

bool ESCAASTVisitor::ProcessFunction( clang::FunctionDecl *f )
{
    std::string funName = f->getNameInfo().getName().getAsString();

    ////////////
    Cout << "Visiting function " << funName << " id: " << f->getBuiltinID() << "\n";
    auto loc = f->getLocation();
    Cout << "Location:\n";
    Cout << "\tloc.getRawEncoding(): " << loc.getRawEncoding() << " loc.isFileID(): " << loc.isFileID() << "\n";
    clang::SourceManager &sm = f->getASTContext().getSourceManager();
    currSM = &sm;
    loc.print(llvm::nulls(), sm);
    auto lstr = loc.printToString(sm);
    Cout << "\ngetFromPtrEncoding dump:\n";
    clang::SourceLocation::getFromPtrEncoding(loc.getPtrEncoding()).print(llvm::nulls(), sm);
    Cout << "\nlocation string: " << lstr << "\n";

    PathStorage ps(lstr.substr(0, lstr.substr(4).find_first_of(":") + 4));
    if( ps.Folder() != path->Folder())
    {
        return true;
    }

    f->print(llvm::nulls());

    /////////

    auto body = f->getBody();
    if( body != nullptr )
    {
        if( !f->isGlobal())
        {
            static int num = 0;
            staticFuncMapping[ funName ] = funName + std::to_string(num);

            funName = funName + std::to_string(num);
            num++;
        }

        fsm.FunctionName(funName);

        ctx.addFoo();
        ctx.lastFoo->name = funName;

        allFunctions[ funName ] = ctx.lastFoo;

        ProcessStmt(body);
        // lazy
        //ProcessReturnNone();

        Reset();
    }

    f->print(llvm::nulls());

    return true;
}

void ESCAASTVisitor::Reset()
{
    fsm.Reset();
    variables.clear();
}

bool ESCAASTVisitor::ProcessStmt( clang::Stmt *stmt, bool addToStates )
{
    if( !stmt )
    {
        return true;
    }

    Cout << "~~Processing statement~~\n";
    //stmt->dump();
    using namespace clang;
    if( auto compSt = dyn_cast<CompoundStmt>(stmt))
    {
        ProcessCompound(compSt, addToStates);
    }
    else if( auto declSt = dyn_cast<DeclStmt>(stmt))
    {
        Cout << "DeclStmt!\n";
        if( declSt->isSingleDecl())
        {
            auto sd = declSt->getSingleDecl();
            if( sd )
            {
                if( auto varSt = dyn_cast<VarDecl>(sd))
                {
                    Cout << "VarDecl!\n";
                    ProcessDeclaration(varSt);
                }
            }
        }
        else
        {
            DeclGroupRef group = declSt->getDeclGroup();
            if( !group.isNull())
            {
                Cout << "DeclGroup\n";
                //TODO: do something!
            }
        }
    }
    else if( isa<BinaryOperator>(stmt))
    {
        auto bo = cast<BinaryOperator>(stmt);
        if( bo->isAssignmentOp())
        {
            ProcessAssignment(bo);
        }
    }
    else if( auto delSt = dyn_cast<CXXDeleteExpr>(stmt))
    {
        Cout << "delete!\n";
        ProcessDelete(delSt);
    }
    else if( auto retSt = dyn_cast<ReturnStmt>(stmt))
    {
        ProcessReturn(retSt);
    }
    else if( auto ifSt = dyn_cast<IfStmt>(stmt))
    {
        ProcessIf(ifSt);
    }
    else if( auto castSt = dyn_cast<ParenExpr>(stmt))
    {
        auto sub = castSt->getSubExpr();
        if( auto bo = dyn_cast<BinaryOperator>(sub))
        {
            if( bo->isAssignmentOp())
            {
                ProcessAssignment(bo);
            }
        }
    }
    else if( auto rhsRefExpr = dyn_cast<CallExpr>(stmt))
    {
        auto callee = rhsRefExpr->getCallee();
        //callee->dump();
        if( auto subExpr = dyn_cast<ImplicitCastExpr>(callee))
        {
            if( auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr()))
            {
                std::string fooName = foo->getNameInfo().getName().getAsString();

                if( fooName == "free" )
                {
                    auto x = rhsRefExpr->getArg(0);
                    std::string varName;

                    if( auto im = dyn_cast<ImplicitCastExpr>(x))
                    {
                        if( auto im2 = dyn_cast<ImplicitCastExpr>(im->getSubExpr()))
                        {
                            auto x2 = im2->getSubExpr()->getStmtClassName();
                            if( auto d = dyn_cast<DeclRefExpr>(im2->getSubExpr()))
                            {
                                varName = d->getNameInfo().getName().getAsString();
                            }
                        }
                    }

                    if( staticFuncMapping.find(fooName) != staticFuncMapping.end())
                    {
                        fooName = staticFuncMapping[ fooName ];
                    }

                    ctx.addDeleteStatement(varName, false);
                }
            }
        }
    }

    return true;
}

bool ESCAASTVisitor::ProcessCompound( clang::CompoundStmt *body, bool addToStates )
{
    Target::Statement *state = ctx.createCompoundStatement(addToStates);

    ctx.setIf(state);

    auto iter = body->body_begin();
    for( ; iter != body->body_end(); ++iter )
    {
        ProcessStmt(*iter);
    }

    ctx.addCompoundStatement();
    ctx.popCompound();

    return true;
}

bool ESCAASTVisitor::ProcessAssignment( clang::BinaryOperator *binop )
{
    // blablabla
    Cout << "assignment \n";
    using namespace clang;

    Stmt *lhs = binop->getLHS();

//    auto name = lhs->getStmtClassName();

    if( isa<DeclRefExpr>(lhs))
    {
        Cout << "\tto reference ";
        auto *ref = cast<DeclRefExpr>(lhs);
        DeclarationNameInfo nameInfo = ref->getNameInfo();
        DeclarationName name = nameInfo.getName();
        //name.dump();
        std::string sname = name.getAsString();
        Cout << sname;

        PtrCounter &lhsCnt = variables[ sname ];
        int lhsVer = ++(lhsCnt.count);

        Stmt *rhs = binop->getRHS();

        //rhs->dump();

        if( isa<CXXNewExpr>(rhs))
        {
            auto newOp = cast<CXXNewExpr>(rhs);

            newOp->getBeginLoc();

            ctx.addVarAssigmentNew(sname, newOp->isArray(), getLocation(newOp));
        }
        if( isa<ImplicitCastExpr>(rhs))
        {
            auto ice = cast<ImplicitCastExpr>(rhs);
            Stmt *subexpr = ice->getSubExpr();
            if( isa<DeclRefExpr>(subexpr)) //pointer
            {
                auto rhsRefExpr = cast<DeclRefExpr>(subexpr);
                std::string rhsPointer = rhsRefExpr->getNameInfo().getName().getAsString();

                ctx.addVarAssigmentFromPointer(sname, rhsPointer, getLocation(rhsRefExpr));
            }
        }
        if( isa<CallExpr>(rhs))
        {
            auto rhsRefExpr = cast<CallExpr>(rhs);
            auto callee = rhsRefExpr->getCallee();
            //callee->dump();
            if( auto subExpr = dyn_cast<ImplicitCastExpr>(callee))
            {
                if( auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr()))
                {
                    std::string fooName = foo->getNameInfo().getName().getAsString();

                    if( staticFuncMapping.find(fooName) != staticFuncMapping.end())
                    {
                        fooName = staticFuncMapping[ fooName ];
                    }

                    ctx.lastFoo->callee.push_back(fooName);
                    ctx.addVarAssigmentFromFoo(sname, fooName, getLocation(foo));
                }
            }
        }
        if( auto castSt = dyn_cast<CastExpr>(rhs))
        {
            if( auto rhsRefExpr = dyn_cast<CallExpr>(castSt->getSubExpr()))
            {
                auto callee = rhsRefExpr->getCallee();
                //callee->dump();
                if( auto subExpr = dyn_cast<ImplicitCastExpr>(callee))
                {
                    if( auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr()))
                    {
                        std::string fooName = foo->getNameInfo().getName().getAsString();

                        if( staticFuncMapping.find(fooName) != staticFuncMapping.end())
                        {
                            fooName = staticFuncMapping[ fooName ];
                        }

                        ctx.lastFoo->callee.push_back(fooName);
                        ctx.addVarAssigmentFromFoo(sname, fooName, getLocation(foo));
                    }
                }
            }
        }
        //ref->get
    }
    return true;
}

bool ESCAASTVisitor::ProcessDeclaration( clang::VarDecl *vd )
{
    Cout << "\n\n";
    vd->print(llvm::nulls());

    auto tmp1 = vd->getTypeSourceInfo();
    auto tmp = vd->getTypeSourceInfo()->getType();
    auto tmp2 = vd->getTypeSourceInfo()->getType().getTypePtr();

    if( !vd->getTypeSourceInfo()->getType().getTypePtr()->isAnyPointerType())
    {
        return true;
    }

    Cout << "isBlockPointerType: "
         << vd->getTypeSourceInfo()->getType().getTypePtr()->isBlockPointerType()
         << "\n";

    Cout << "isAggregateType: "
         << vd->getTypeSourceInfo()->getType().getTypePtr()->isAggregateType()
         << "\n";

    Cout << "isAnyComplexType: "
         << vd->getTypeSourceInfo()->getType().getTypePtr()->isAnyComplexType()
         << "\n";

    Cout << "isArrayType: "
         << vd->getTypeSourceInfo()->getType().getTypePtr()->isArrayType()
         << "\n";

    Cout << "isDependentSizedArrayType: "
         << vd->getTypeSourceInfo()->getType().getTypePtr()->isDependentSizedArrayType()
         << "\n";

    Cout << "isIntegerType: "
         << vd->getTypeSourceInfo()->getType().getTypePtr()->isIntegerType()
         << "\n";

    Cout << "type: " << vd->getTypeSourceInfo()->getType().getAsString();

    Cout << "\n\n";
    using namespace clang;


    if( vd->getTypeSourceInfo()->getType().getTypePtr()->isAnyPointerType())
    {
        auto name = vd->getNameAsString();
        auto iter = variables.find(name);
        if( iter != variables.end())
        {
            //Error: this variable is already declared.
            Cout << "Variable with name " << name << " declared twice\n";
            return true;
        }
        PtrCounter ptrCnt{
                .count = 0,
                .meta = VAR_POINTER
        };
        //variables[name] = ptrCnt;
        auto cntIter = variables.insert(std::pair<std::string, PtrCounter>(name, ptrCnt));

        auto init = vd->getAnyInitializer();
        if( init == nullptr )
        {
            return true;
        }
        if( !isa<CXXNewExpr>(init))
        {
            if( auto castSt = dyn_cast<CastExpr>(init))
            {
                init = castSt->getSubExpr();
            }

            if( auto rhsRefExpr = dyn_cast<CallExpr>(init))
            {
                auto callee = rhsRefExpr->getCallee();
                //callee->dump();
                if( auto subExpr = dyn_cast<ImplicitCastExpr>(callee))
                {
                    if( auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr()))
                    {
                        std::string fooName = foo->getNameInfo().getName().getAsString();

                        if( staticFuncMapping.find(fooName) != staticFuncMapping.end())
                        {
                            fooName = staticFuncMapping[ fooName ];
                        }

                        ctx.lastFoo->callee.push_back(fooName);
                        ctx.addVarDeclFromFoo(name, fooName, getLocation(foo));
                    }
                }
            }
            return false;
        }

        ++cntIter.first->second.count;

        auto newExpr = cast<CXXNewExpr>(init);

        ctx.addVarDeclNew(name, newExpr->isArray(), getLocation(newExpr));
    }
    return true;
}

bool ESCAASTVisitor::ProcessDelete( clang::CXXDeleteExpr *del )
{
    using namespace clang;

    auto argDel = del->getArgument();
    //TODO: change this if by function that gives a full name of pointer or null when there is no pointer.
    if( isa<ImplicitCastExpr>(argDel))
    {
        auto delCast = cast<ImplicitCastExpr>(argDel);
        auto dexpr = delCast->getSubExpr();
        if( isa<DeclRefExpr>(dexpr))
        {
            auto dptr = cast<DeclRefExpr>(dexpr);
            std::string name = dptr->getNameInfo().getAsString();

            ctx.addDeleteStatement(name, del->isArrayForm());
        }
    }
    return true;
}

bool ESCAASTVisitor::ProcessReturn( clang::ReturnStmt *ret )
{
    clang::Expr *retVal = ret->getRetValue();

    returnExpr = retVal;

    std::string returnVarName2;

    //retVal->dump();
    using namespace clang;

    if( retVal && isa<ImplicitCastExpr>(retVal))
    {
        auto ice = cast<ImplicitCastExpr>(retVal);
        Stmt *subexpr = ice->getSubExpr();
        if( isa<DeclRefExpr>(subexpr)) //pointer
        {
            auto ref = cast<DeclRefExpr>(subexpr);
            DeclarationNameInfo nameInfo = ref->getNameInfo();
            DeclarationName name = nameInfo.getName();
            //name.dump();
            std::string sname = name.getAsString();

            ctx.lastFoo->returnName.insert(sname);

            returnVarName2 = sname;
        }
    }

    ctx.addReturn(returnVarName2);

    return /*res*/true;
}

bool ESCAASTVisitor::ProcessReturnNone()
{
    fsm.ProcessReturnNone();
    return true;
}

bool ESCAASTVisitor::ProcessReturnPtr( clang::ReturnStmt *ret )
{
    auto retVal = ret->getRetValue();

    return true;
}

bool ESCAASTVisitor::ProcessIf( clang::IfStmt *ifstmt )
{
    Cout << "If statement!\n";
    using namespace clang;

    auto cond = ifstmt->getCond();
    {
        auto curCond = cond;
        if( auto c = dyn_cast<UnaryOperator>(curCond))
        {
            curCond = c->getSubExpr();
        }

        if( auto castSt = dyn_cast<CastExpr>(curCond))
        {
            auto sub4 = castSt->getSubExpr();
            if( auto castSt3 = dyn_cast<CastExpr>(sub4))
            {
                auto sub3 = castSt3->getSubExpr();
                if( auto parenSt = dyn_cast<ParenExpr>(sub3))
                {
                    auto sub = parenSt->getSubExpr();
                    if( auto bo = dyn_cast<BinaryOperator>(sub))
                    {
                        if( bo->isAssignmentOp())
                        {
                            ProcessAssignment(bo);
                        }
                    }
                }
            }
        }
    }
    std::string typeS;
    llvm::raw_string_ostream lso(typeS);
    clang::LangOptions langOpts;
    langOpts.CPlusPlus11 = true;
    PrintingPolicy pol(langOpts);
    cond->printPretty(lso, nullptr, pol);
    std::string condStr = "~if - " + lso.str();
    std::string elseStr = "~else - " + condStr;
    Cout << "\tCondition: " << condStr << "\n";
    //TODO: Create the state branching.
    Target::Statement *thenSt = nullptr;
    ctx.startIfSt(&thenSt);
    ProcessStmt(ifstmt->getThen(), false);

    Target::Statement *elseSt = nullptr;
    ctx.startIfSt(&elseSt);
    ProcessStmt(ifstmt->getElse(), false);

    ctx.addIfStatement(thenSt, elseSt, condStr, elseStr);

    return true;
}
