#include <string>

#include <llvm/Support/raw_ostream.h>

#include "ESCAASTVisitor.h"
#include "ASTWalker.h"

#include "Variable.h"

using namespace clang;
using namespace std;

ESCAASTVisitor::ESCAASTVisitor() : walker(0), insideMain(false)
{
}

bool ESCAASTVisitor::VisitStmt(clang::Stmt *s)
{
	if (!insideMain)
	{
		return false;
	}

	llvm::errs() << "Visiting statement\n";  
	walker->DumpStmt(s);
		
	if (clang::isa<clang::Expr>(s))
	{
		llvm::errs() << "\t Expression ";

		//s->printPretty(llvm::errs, 
		if (clang::isa<clang::BinaryOperator>(s)) 
		{
			llvm::errs() << ": binary operator: ";
			BinaryOperator *binop = cast<BinaryOperator>(s);
			if (cast<BinaryOperator>(s)->isAssignmentOp() == true) 
			{
				// blablabla
				llvm::errs() << "assignment \n";

				Stmt *lhs = binop->getLHS();
				if (isa<DeclRefExpr>(lhs))
				{
					llvm::errs() << "\tto reference ";
					DeclRefExpr *ref = cast<DeclRefExpr>(lhs);
					DeclarationNameInfo nameInfo = ref->getNameInfo();
					DeclarationName name = nameInfo.getName();
					//name.dump();
					string sname = name.getAsString();
					llvm::errs() << sname;
					
					/*
					if (variables.find(sname) == variables.end())
					{
						VersionedVariable vv(sname, "", VAR_POINTER, 0);
						std::vector<VersionedVariable> vvvector;
						vvvector.push_back(vv);
						variables[sname] = vvvector;
						//variables.insert(
					}
					*/
					++variables[sname];

					//std::vector<VersionedVariable> &vvvector = variables[sname];

					Stmt *rhs = binop->getRHS();
					if (isa<CXXNewExpr>(rhs))
					{
						CXXNewExpr *newOp = cast<CXXNewExpr>(rhs);
						if (newOp->isArray())
						{
							std::string type = "";
							VersionedVariable vv(type, sname, VAR_ARRAY_POINTER, variables[sname]);
							allocated.push_back(vv);
						}
						//�������� new.
					}
					if (isa<ImplicitCastExpr>(rhs))
					{
						ImplicitCastExpr *ice = cast<ImplicitCastExpr>(rhs);
						Stmt *subexpr = ice->getSubExpr();
						if (isa<DeclRefExpr>(subexpr))
						{
							//AddToSolver(sname, variables[sname], ...);
						}
					}

					//ref->get
				}
			}
		}

		if (isa<clang::CXXNewExpr>(s))
		{
			llvm::errs() << ": new operator: ";
			CXXNewExpr *newExpr = cast<CXXNewExpr>(s);
			if (newExpr->isArray())
			{
				QualType qt = newExpr->getAllocatedType();
				Expr *size = newExpr->getArraySize();

					
				llvm::errs() << "array of " << qt.getAsString();

				//size->dump( (errs);
				//if (size->isConstantInitializer())
				{

				}
					
			}
		}
	}
    return true;
}

bool ESCAASTVisitor::VisitBinaryOperator(BinaryOperator* bo) 
{
	if (!insideMain)
	{
		return false;
	}

    if (bo->isAssignmentOp() == true) 
	{
        llvm::errs() << "Visiting assignment ";
        Expr *LHS;
        LHS = bo->getLHS();
        DeclRefExpr* dre;
        if ((dre = dyn_cast<DeclRefExpr>(LHS))) { 
            string name = (dre->getNameInfo()).getName().getAsString();
            llvm::errs() << "to " << name;
        }
        if (ArraySubscriptExpr* ase = dyn_cast<ArraySubscriptExpr>(LHS)) { 
            Expr *arrayBase = ase->getBase()->IgnoreParenCasts();
            if ((dre = dyn_cast<DeclRefExpr>(arrayBase))) { 
                string name = (dre->getNameInfo()).getName().getAsString();
                llvm::errs() << "to array " << name;
            }
        }
        llvm::errs() << "\n";
    }
    return true;
}

bool ESCAASTVisitor::shouldVisitTemplateInstantiations() const 
{
	if (!insideMain)
	{
		return false;
	}

    llvm::errs() << "PIPPOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO" << "\n";
    return true; 
}

bool ESCAASTVisitor::VisitCXXOperatorCallExprs(CXXOperatorCallExpr *e) 
{
	if (!insideMain)
	{
		return false;
	}

	llvm::errs() << "Visiting cxxoperatorcall" << "\n";
    return true;
}

bool ESCAASTVisitor::VisitCXXConstructorDecl(CXXConstructorDecl *c) 
{
	if (!insideMain)
	{
		return false;
	}

    llvm::errs() << "Visiting CXXConstructorDecl" << "\n";
    return true;        
}

bool ESCAASTVisitor::VisitDeclRefExpr(DeclRefExpr* expr) 
{
	if (!insideMain)
	{
		return false;
	}

    string name = (expr->getNameInfo()).getName().getAsString();
    llvm::errs() << name << "\n";
    return true;
}

bool ESCAASTVisitor::VisitVarDecl(VarDecl *v) 
{
	if (!insideMain)
	{
		return false;
	}

	llvm::errs() << "Visiting declaration of variable " << v->getDeclName().getAsString() << "\n";
    llvm::errs() << "  type: " << v->getTypeSourceInfo()->getType().getTypePtr()->getTypeClassName();
    if (v->getTypeSourceInfo()->getType().getTypePtr()->isFloatingType() == true) 
	{
        llvm::errs() << " -> float";
    }
    if(v->getTypeSourceInfo()->getType().getTypePtr()->isConstantArrayType() == true) 
	{
        llvm::errs() << " of ";
        llvm::errs() << v->getTypeSourceInfo()->getType().getAsString();
        llvm::errs() << " size ";
        llvm::APInt arraySize = cast<ConstantArrayType>(v->getTypeSourceInfo()->getType().getTypePtr())->getSize();
        llvm::errs() << arraySize;
    }
    if(v->getTypeSourceInfo()->getType().getTypePtr()->isPointerType() == true) 
	{
        llvm::errs() << " to " << v->getTypeSourceInfo()->getType().getAsString();

    }
    llvm::errs() << "\n";
    return true;
}

bool ESCAASTVisitor::VisitTypedefDecl(clang::TypedefDecl *d) 
{
    //llvm::errs() << "Visiting " << d->getDeclKindName() << " " << d->getName() << "\n";

    return true; // returning false aborts the traversal        
}

bool ESCAASTVisitor::VisitFunctionDecl(FunctionDecl *f) 
{
	string funName = f->getNameInfo().getName().getAsString();
	if (funName != "main")
	{
		return false;
	}

	insideMain = true;
    llvm::errs() << "Visiting function " << funName << "\n";
	f->dump();

    return true;
}
