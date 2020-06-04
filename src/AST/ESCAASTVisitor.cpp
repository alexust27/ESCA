#include <string>
#include <map>
#include <vector>
#include <set>

#include <llvm/Support/raw_ostream.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/ASTContext.h>

#include "../target/Context.h"
//#include <z3++.h>
#include <DefectStorage.h>

#include "ESCAASTVisitor.h"

#include "Output.h"

#define context Target::Context::Instance()

bool ESCAASTVisitor::VisitFunctionDecl( clang::FunctionDecl *f )
{
    isInDestruct = false;
    isInConstructor = false;
    if( clang::isa<clang::CXXConstructorDecl>(f))
    {
        isInConstructor = true;
    }
    if( clang::isa<clang::CXXDestructorDecl>(f))
    {
        isInDestruct = true;
    }

    return ProcessFunction(f);
}

//bool ESCAASTVisitor::VisitValueDecl( clang::ValueDecl *VD )
//{
////    if( auto fld = clang::dyn_cast<clang::FieldDecl>(VD))
////    {
////        auto classVarName = fld->getDeclName().getAsString();
////        auto className = fld->getQualifiedNameAsString();
////        className.resize(className.length() - classVarName.length() - 2);
////        classToVars[ className ].insert(classVarName);
////    }
//    return RecursiveASTVisitor::VisitValueDecl(VD);
//}


std::string ESCAASTVisitor::getLocation( const clang::Stmt *st )
{
    auto loc = st->getBeginLoc();
    return loc.printToString(*currSM);
}

bool ESCAASTVisitor::ProcessFunction( clang::FunctionDecl *functionDecl )
{
    astContext = &functionDecl->getASTContext();
    std::string funName = functionDecl->getNameInfo().getName().getAsString();

    ////////////
    clang::SourceManager &sm = functionDecl->getASTContext().getSourceManager();
    currSM = &sm;
    auto lstr = functionDecl->getLocation().printToString(sm);

//    f->dump();
    auto body = functionDecl->getBody();
    if( body != nullptr )
    {
        if( !functionDecl->isGlobal())
        {
            static int num = 0;
            staticFuncMapping[ funName ] = funName + std::to_string(num);

            funName = funName + std::to_string(num);
            ++num;
        }

        if( funName == "main" )
        {
            static int numMain = 0;
            funName = funName + "&" + std::to_string(numMain);
            ++numMain;
        }

        context.AddFunction(funName);

        /// MAIN FUNCTION
        ProcessStmt(body);

        if( context.CatchException())
        {
            if( isInDestruct || isInConstructor )
            {
                DefectStorage::Instance().AddDefect(functionDecl->getQualifiedNameAsString(), lstr);
            }
            context.throwsFunctions.insert({funName, context.GetException()});
            context.curFunction->SetException(context.GetException());
        }

        // lazy
        //ProcessReturnNone();
        context.ResetFunction();
//        allVariables.clear();
//        variableToExpr.clear();
//        localVariables.clear();
    }

    return true;
}


bool ESCAASTVisitor::ProcessStmt( clang::Stmt *stmt )
{
    if( !stmt )
    {
        return true;
    }

    //stmt->dump();
    using namespace clang;

    if( auto compSt = dyn_cast<CompoundStmt>(stmt))
    {
        ProcessCompound(compSt);
    }
    else if( auto declSt = dyn_cast<DeclStmt>(stmt))
    {
        if( declSt->isSingleDecl())
        {
            auto sd = declSt->getSingleDecl();
            if( sd )
            {
                if( auto varSt = dyn_cast<VarDecl>(sd))
                {
                    ProcessDeclaration(varSt);
                }
            }
        }
        else
        {
            DeclGroupRef group = declSt->getDeclGroup();
            if( !group.isNull())
            {
                for( auto v : group )
                {
                    ProcessDeclaration(dynamic_cast<VarDecl *>(v));
                }
            }
        }
    }
    else if( isa<BinaryOperator>(stmt))
    {
        auto bo = cast<BinaryOperator>(stmt);
        if( bo->isAssignmentOp())
        {
            ProcessBinOp(bo);
        }
    }
    else if( auto castSt = dyn_cast<ParenExpr>(stmt))
    {
        if( auto bo = dyn_cast<BinaryOperator>(castSt->getSubExpr()))
        {
            if( bo->isAssignmentOp())
            {
                ProcessBinOp(bo);
            }
        }
    }
    else if( auto delSt = dyn_cast<CXXDeleteExpr>(stmt))
    {
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
    else if( auto tr = dyn_cast<CXXTryStmt>(stmt))
    {
        ProcessTry(tr);
    }
    else if( auto exWithClUp = dyn_cast<ExprWithCleanups>(stmt))
    {
        auto thr = exWithClUp->children().begin();
        if( auto throwSt = dyn_cast<CXXThrowExpr>(*thr))
        {
            ProcessThrow(throwSt);
        }
    }
    else if( auto cycle = dyn_cast<WhileStmt>(stmt))
    {
//        std::cout << "while" << std::endl;
        ProcessStmt(cycle->getBody());
    }
    else if( auto forCycle = dyn_cast<ForStmt>(stmt))
    {
        auto cond = dyn_cast<BinaryOperator>(forCycle->getCond());
        if( cond )
        {
            bool flag = cond->getRHS()->isIntegerConstantExpr(*astContext);
            if( flag )
            {
                Expr::EvalResult res;
                int64_t tmp = context.ForCounter;
                cond->getRHS()->EvaluateAsInt(res,*astContext);
                context.ForCounter = tmp * (res.Val.getInt().getExtValue());
                ProcessStmt(forCycle->getBody());
                context.ForCounter = tmp;
            }
        }
        else
        {
            ProcessStmt(forCycle->getBody());
        }

    }
    else if( auto rhsRefExpr = dyn_cast<CallExpr>(stmt))
    {
        ProcessCallFunction(rhsRefExpr);
    }

    return true;
}

bool ESCAASTVisitor::ProcessCompound( clang::CompoundStmt *body, bool createOnStack )
{
//    auto saveVars = allVariables;
    if( createOnStack )
    {
        context.CreateCompoundStatement();
    }

//    context.setIf(state);
//    body->dump();
    for( auto iter = body->body_begin(); iter != body->body_end(); ++iter )
    {
        ProcessStmt(*iter);
        if( context.CatchException())
        {
            break;
        }
    }

//    allVariables = saveVars; // удаляем локальные переменные

    if( createOnStack )
    {
        context.PopCompound();
    }

    return true;
}

bool ESCAASTVisitor::ProcessCallFunction( clang::CallExpr *rhsRefExpr )
{
    using namespace clang;

    auto callee = rhsRefExpr->getCallee();
    if( auto memExpr = dyn_cast<MemberExpr>(callee))
    {
        std::string fooName = memExpr->getMemberNameInfo().getName().getAsString();
        EditFunName(fooName);
        if( context.throwsFunctions.count(fooName))
        {
            context.CreateThrow(context.throwsFunctions[ fooName ]);
            return true;
        }
    }
    if( auto subExpr = dyn_cast<ImplicitCastExpr>(callee))
    {
        if( auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr()))
        {
            std::string fooName = foo->getNameInfo().getName().getAsString();
//            std::string fooName = foo->getQualifier()->getAsNamespace()->getQualifiedNameAsString();
            if( context.IsFreeFunction(fooName))
            {
                auto x = rhsRefExpr->getArg(0);
                std::string varName;

                if( auto im = dyn_cast<ImplicitCastExpr>(x))
                {
                    if( auto im2 = dyn_cast<ImplicitCastExpr>(im->getSubExpr())) // for free
                    {
                        if( auto d = dyn_cast<DeclRefExpr>(im2->getSubExpr()))
                        {
                            varName = d->getNameInfo().getName().getAsString();
                        }
                    }
                    else // for sockets
                    {
                        if( auto d = dyn_cast<DeclRefExpr>(im->getSubExpr()))
                        {
                            varName = d->getNameInfo().getName().getAsString();
                        }
                    }
                }

                context.AddToLast(new Target::DeleteStatement(varName, false));
            }
        }
    }
    return true;
}

void ESCAASTVisitor::AddVarDeclFromFoo( const std::string &varName, std::string &fooName, const std::string &location,
                                        bool isDecl )
{
    EditFunName(fooName);
    context.curFunction->callee.push_back(fooName);
    if( context.throwsFunctions.count(fooName))
    {
        context.CreateThrow(context.throwsFunctions[ fooName ]);
        return;
    }
    context.AddToLast(
            new Target::VarAssigmentFromFooStatement(varName, fooName, location, isDecl));
}

bool ESCAASTVisitor::ProcessAssignment( const clang::Stmt *init, const std::string &lhsName, bool isDecl )
{
    using namespace clang;
    // для new
    // пример: int *x = new int[10]
    // или
    // пример: x = new int[10]
    if( auto newOp = dyn_cast<CXXNewExpr>(init))
    {
        context.AddToLast(
                new Target::VarAssigmentNewStatement(lhsName, newOp->isArray(), getLocation(newOp), isDecl));
        return true;
    }

    if( auto castSt = dyn_cast<CastExpr>(init))
    {
        init = castSt->getSubExpr();
    }
    if( auto castSt = dyn_cast<ImplicitCastExpr>(init))
    {
        init = castSt->getSubExpr();
    }


    // для указателей
    // пример:
    // int *x, *a;
    // x = a;
    // или
    // int * x = a;
    if( auto rhsPointer = dyn_cast<DeclRefExpr>(init))
    {
        std::string rhsPointerName = rhsPointer->getNameInfo().getName().getAsString();

        context.AddToLast(
                new Target::VarAssigmentFromPointerStatement(lhsName, rhsPointerName, getLocation(rhsPointer), isDecl));
    }
    // для функций
    // пример: int *x = ret_int()
    //         x = ret_int()
    if( auto rhsRefExpr = dyn_cast<CallExpr>(init))
    {
        if( auto subExpr = dyn_cast<ImplicitCastExpr>(rhsRefExpr->getCallee()))
        {
            if( auto foo = dyn_cast<DeclRefExpr>(subExpr->getSubExpr()))
            {
                std::string fooName = foo->getNameInfo().getName().getAsString();
//                std::string fooName = foo->getQualifier()->getAsNamespace()->getQualifiedNameAsString();
                AddVarDeclFromFoo(lhsName, fooName, getLocation(foo), isDecl);
                return true;
            }
        }
    }

    // для методов
    // пример: int *x = a.retPointerInt()
    if( auto metCall = dyn_cast<CXXMemberCallExpr>(init))
    {
        if( auto metExpr = dyn_cast<MemberExpr>(metCall->getCallee()))
        {
            auto fooName = metExpr->getMemberDecl()->getNameAsString();
            AddVarDeclFromFoo(lhsName, fooName, getLocation(metExpr), isDecl);
            return true;
        }
    }
    //ref->get
    return true;

}

bool ESCAASTVisitor::EvaluateBool( const clang::Stmt *init, bool &res )
{
    return true;
}

bool ESCAASTVisitor::ProcessDeclaration( clang::VarDecl *vd )
{
    using namespace clang;
    if( !vd )
    {
        return true;
    }
    auto init = vd->getAnyInitializer();
    if( init == nullptr )
    {
        return true;
    }
    auto varName = vd->getNameAsString();
    if( vd->getType()->isBooleanType())
    {
        bool res;
        if( init->isEvaluatable(vd->getASTContext()))
        {
            init->EvaluateAsBooleanCondition(res, *astContext);
//            z3::expr x = ;
//            variableToExpr[ varName ] = res;
        }
        else
        {
            if( EvaluateBool(init, res))
            {
                //todo
            }

        }
        return true;
    }

//    while( allVariables.find(varName) != allVariables.end())
//    {
//        //   varName += "_";
//        Cout << "Variable with name " << varName << " declared twice\n";
//        return true;
//    }
//    allVariables.insert(varName);


    return ProcessAssignment(init, varName, true);
}

bool ESCAASTVisitor::ProcessBinOp( clang::BinaryOperator *binop )
{
    using namespace clang;

    Stmt *lhs = binop->getLHS();

    std::string lhsName;
    if( auto ref = dyn_cast<DeclRefExpr>(lhs))
    {
        lhsName = ref->getNameInfo().getName().getAsString();
    }
    else if( auto ref2 = dyn_cast<MemberExpr>(lhs))
    {
        lhsName = ref2->getMemberNameInfo().getName().getAsString();
    }
    else
    {
//        lhs->dump();
        return false;
    }

    const Stmt *rhs = binop->getRHS();

    return ProcessAssignment(rhs, lhsName, false);
}

bool ESCAASTVisitor::ProcessDelete( clang::CXXDeleteExpr *del )
{
    using namespace clang;
//    del->dump();
    auto argDel = del->getArgument();
    //TODO: change this if by function that gives a full name of pointer or null when there is no pointer.
    if( auto delCast = dyn_cast<ImplicitCastExpr>(argDel))
    {
        if( auto delPtr = dyn_cast<DeclRefExpr>(delCast->getSubExpr()))
        {
            std::string varName = delPtr->getNameInfo().getAsString();

            context.AddToLast(new Target::DeleteStatement(varName, del->isArrayForm()));
        }
    }
    return true;
}

bool ESCAASTVisitor::ProcessReturn( clang::ReturnStmt *ret )
{
    clang::Expr *retVal = ret->getRetValue();
    if( !retVal )
    {
        return true;
    }

    std::string returnVarName2;

    using namespace clang;
    Stmt *subexpr = retVal;
    while( isa<ImplicitCastExpr>(subexpr))
    {
        subexpr = cast<ImplicitCastExpr>(subexpr)->getSubExpr();
    }
    if( auto ref = dyn_cast<DeclRefExpr>(subexpr)) //pointer
    {
        std::string retName = ref->getNameInfo().getName().getAsString();

        context.curFunction->returnName.insert(retName);

        returnVarName2 = retName;
    }
    else if( auto newPtr = dyn_cast<CXXNewExpr>(subexpr))
    {
        std::string dummyNewVarName = "!dummyVar";
        context.curFunction->returnName.insert(dummyNewVarName);
        context.AddToLast(new Target::VarAssigmentNewStatement(dummyNewVarName, newPtr->isArray(), "something", true));
        returnVarName2 = dummyNewVarName;
    }

    context.AddToLast(new Target::ReturnStatement(returnVarName2));

    return true;
}

bool ESCAASTVisitor::ProcessIf( clang::IfStmt *ifstmt )
{
    Cout << "If statement!\n";
//    ifstmt->dump();
    auto cond = ifstmt->getCond();
//    cond->dump();
//    {
    auto curCond = cond;
    if( auto c = clang::dyn_cast<clang::UnaryOperator>(curCond))
    {
        curCond = c->getSubExpr();
    }

    if( auto castSt = clang::dyn_cast<clang::CastExpr>(curCond))
    {
        auto sub4 = castSt->getSubExpr();
        if( auto castSt3 = clang::dyn_cast<clang::CastExpr>(sub4))
        {
            auto sub3 = castSt3->getSubExpr();
            if( auto parenSt = clang::dyn_cast<clang::ParenExpr>(sub3))
            {
                auto sub = parenSt->getSubExpr();
                if( auto bo = clang::dyn_cast<clang::BinaryOperator>(sub))
                {
                    if( bo->isAssignmentOp())
                    {
                        ProcessBinOp(bo);
                    }
                }
            }
        }
    }
//    }
    std::string typeS;
    llvm::raw_string_ostream lso(typeS);
    clang::LangOptions langOpts;
    langOpts.CPlusPlus17 = true;
    clang::PrintingPolicy pol(langOpts);
    cond->printPretty(lso, nullptr, pol);
    std::string condStr = "~if - " + lso.str();
    std::string elseStr = "~else - " + condStr;
//    std::cout << "\tCondition: " << condStr << "\n";
    if( cond->isEvaluatable(*astContext))
    {
        //todo
    }


    if( !context.CreateIfStatement(ifstmt->hasElseStorage(), condStr, elseStr))
    {
        return false;
    }
    if( auto thenSt = clang::dyn_cast<clang::CompoundStmt>(ifstmt->getThen()))
    {
        ProcessCompound(thenSt, false);
    }
    else
    {
//        ifstmt->dump();
        ProcessStmt(ifstmt->getThen());
    }

    context.SwitchToElse();
    if( ifstmt->hasElseStorage() && !context.CatchException())
    {
        if( auto elseSt = clang::dyn_cast<clang::CompoundStmt>(ifstmt->getElse()))
        {
            ProcessCompound(elseSt, false);
        }
        else
        {
//            ifstmt->getElse()->dump();
            ProcessStmt(ifstmt->getElse());
        }
        context.PopCompound();
    }

    return true;
}

void ESCAASTVisitor::ProcessThrow( clang::CXXThrowExpr *stmt )
{
    if( auto constrExp = clang::dyn_cast<clang::CXXConstructExpr>(stmt->getSubExpr()))
    {
        auto exceptionName = constrExp->getConstructor()->getNameAsString();
        context.CreateThrow(exceptionName);
    }
}

void ESCAASTVisitor::ProcessTry( clang::CXXTryStmt *stmt )
{
    //        std::cout << "try expr" << std::endl;
//    stmt->dump();
    if( !context.CreateTryStatement())
    {
        return;
    }
    if( auto trBlock = clang::dyn_cast<clang::CompoundStmt>(stmt->getTryBlock()))
    {
        ProcessCompound(trBlock, false);
    }
    if( context.CatchException())
    {
        auto eName = context.GetException();
        for( size_t i = 0; i < stmt->getNumHandlers(); ++i )
        {
            auto exceptionCatchStr = stmt->getHandler(i)->getCaughtType().getAsString();
            if( exceptionCatchStr.find(eName) != std::string::npos ||
                exceptionCatchStr.find("std::exception") != std::string::npos )
            {
                context.CreateCatchStatement();
                auto catchSt = clang::cast<clang::CompoundStmt>(stmt->getHandler(i)->getHandlerBlock());
                ProcessCompound(catchSt, false);
                context.CreateThrow(""); // очищаем исключение, оно поймано
                assert(!context.CatchException());
                break;
            }
        }
    }
    context.PopCompound(); // убираем tryBlock или catch
}

void ESCAASTVisitor::EditFunName( std::string &funName )
{
    if( staticFuncMapping.find(funName) != staticFuncMapping.end())
    {
        funName = staticFuncMapping[ funName ];
    }
}

//bool ESCAASTVisitor::ProcessReturnNone()
//{
//    fsm.ProcessReturnNone();
//    return true;
//}

//bool ESCAASTVisitor::ProcessReturnPtr( clang::ReturnStmt *ret )
//{
//    auto retVal = ret->getRetValue();
//
//    return true;
//}
