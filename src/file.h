#include <vector>
#include <memory>
#include <map>

#include "FSM/FSM.h"
#include "SMT/VariableSMT.h"
#include "SMT/BinarySMT.h"

namespace Target
{
    class Function;
}

extern std::set<std::string> allocatedFunctions;
extern std::map<std::string, Target::Function *> allFunctions;

extern std::set<std::string> processedFunctions;

namespace Target
{

    struct ProcessCtx
    {
        std::map<std::string, PtrCounter> variables;

        std::vector<VersionedVariable> allocated;
        FSM fsm;

        FairLeafPredicate fairPred;
        BranchLeafPredicate branchPred;
    };

    class Statement
    {
    public:
        virtual void process( ProcessCtx &ctx ) = 0;

        virtual ~Statement()
        {
        }
    };


    class DeleteStatement : public Statement
    {
    public:
        DeleteStatement( const std::string &name, bool isArray )
                : name(name), isArray(isArray)
        {
        }

        void process( ProcessCtx &ctx ) override
        {
            auto cntIter = ctx.variables.find(name);
            VersionedVariable vv(name, "unused", cntIter->second.meta, cntIter->second.count);

            ctx.fsm.AddDeleteState(vv, isArray);
        }

    private:
        std::string name;
        bool isArray;
    };


    class CompoundStatement : public Statement
    {
    public:
        CompoundStatement() = default;

        void addState( Statement *st )
        {
            statements.push_back(st);
        }

        void process( ProcessCtx &ctx ) override
        {
            for( auto st : statements )
            {
                st->process(ctx);
            }
        }

    private:
        std::vector<Statement *> statements;
    };


    class IfStatement : public Statement
    {
    public:
        IfStatement( Statement *thenSt, Statement *elseSt, const std::string &condStr, const std::string &elseStr )
                : thenSt(thenSt), elseSt(elseSt), condStr(condStr), elseStr(elseStr)
        {
            static int a = 0;
            a++;
        }

        void process( ProcessCtx &ctx ) override
        {
            // process then
            // скорей всего в then находятся какие то простые действия, котораы нам не интересны
            // например присвоение констант или еще что то такое
            if( thenSt )
            {
                ctx.fsm.PushCondition(condStr);
                StateFSM s;
                ctx.fsm.AddStateToLeaves(s, ctx.fairPred, condStr, false);
                thenSt->process(ctx);
                ctx.fsm.PopCondition();
            }

            // process else
            // тоже самое, как для then, но есть еще случай, когда else вообще отсутствует
            if( elseSt )
            {
                ctx.fsm.PushCondition(elseStr);
                StateFSM s;
                ctx.fsm.AddStateToLeaves(s, ctx.branchPred, elseStr, true);
                elseSt->process(ctx);
                ctx.fsm.PopCondition();
            }
        }

    private:
        Statement *thenSt = nullptr;
        Statement *elseSt = nullptr;
        std::string condStr;
        std::string elseStr;
    };


    class VarDeclFromFooStatement : public Statement
    {
    public:
        VarDeclFromFooStatement( const std::string &varName, const std::string &fooName, const std::string &loc )
                : varName(varName), fooName(fooName), loc(loc)
        {
        }

        void process( ProcessCtx &ctx ) override
        {
            PtrCounter ptrCnt = {
                    0,
                    VAR_POINTER
            };
            //variables[name] = ptrCnt;
            auto cntIter = ctx.variables.insert(std::make_pair(varName, ptrCnt));

            if( allocatedFunctions.find(fooName) != allocatedFunctions.end())
            {

                ++cntIter.first->second.count;

                // TODO: поддержать как царь разные варианты для массива и просто указателя
                StateFSM state;
                VersionedVariable vv(varName, loc, VAR_POINTER, 1);
                {
                    state.allocPointers.push_back(vv);
                }
                //Отметить функцию как new.
                std::shared_ptr<VariableSMT> vvForm(new VariableSMT(vv));
                state.formulae.push_back(vvForm);

                ctx.fsm.AddStateToLeaves(state, ctx.fairPred);
            }
        }

    private:
        std::string varName;
        std::string fooName;
        std::string loc;
    };


    class VarDeclNewStatement : public Statement
    {
    public:
        VarDeclNewStatement( const std::string &varName, bool isArray, const std::string &loc )
                : varName(varName), isArray(isArray), loc(loc)
        {
        }

        void process( ProcessCtx &ctx ) override
        {
            PtrCounter ptrCnt = {
                    0,
                    VAR_POINTER
            };
            //variables[name] = ptrCnt;
            auto cntIter = ctx.variables.insert({varName, ptrCnt});

            ++cntIter.first->second.count;

            StateFSM state;
            VersionedVariable vv(varName, loc, VAR_POINTER, 1);

            if( isArray ) //Declaration of array
            {
                vv.MetaType(VAR_ARRAY_POINTER);
                cntIter.first->second.meta = VAR_ARRAY_POINTER;
                state.allocArrays.push_back(vv);
            }
            else
            {
                state.allocPointers.push_back(vv);
            }
            ctx.allocated.push_back(vv);

            std::shared_ptr<VariableSMT> vvForm(new VariableSMT(vv));
            state.formulae.push_back(vvForm);

            ctx.fsm.AddStateToLeaves(state, ctx.fairPred);
        }

    private:
        std::string varName;
        bool isArray;
        std::string loc;
    };


    class VarAssigmentFromFooStatement : public Statement
    {
    public:
        VarAssigmentFromFooStatement( const std::string &varName, const std::string &fooName, const std::string &loc )
                : varName(varName), fooName(fooName), loc(loc)
        {
        }

        void process( ProcessCtx &ctx ) override
        {
            PtrCounter &lhsCnt = ctx.variables[ varName ];
            int lhsVer = ++(lhsCnt.count);

            if( allocatedFunctions.find(fooName) != allocatedFunctions.end())
            {
                StateFSM state;
                VersionedVariable vv(varName, loc, VAR_POINTER, lhsCnt.count);
                {
                    state.allocPointers.push_back(vv);
                    lhsCnt.meta = VAR_POINTER;
                }
                //Отметить new.
                ctx.allocated.push_back(vv);

                std::shared_ptr<VariableSMT> vvForm(new VariableSMT(vv));
                state.formulae.push_back(vvForm);

                ctx.fsm.AddStateToLeaves(state, ctx.fairPred);
            }
        }

    private:
        std::string varName;
        std::string fooName;
        std::string loc;
    };


    class VarAssigmentFromPointerStatement : public Statement
    {
    public:
        VarAssigmentFromPointerStatement( const std::string &varName, const std::string &rhsName,
                                          const std::string &loc )
                : varName(varName), rhsName(rhsName), loc(loc)
        {
        }

        void process( ProcessCtx &ctx ) override
        {
            PtrCounter &lhsCnt = ctx.variables[ varName ];
            int lhsVer = ++(lhsCnt.count);

            StateFSM state;
            VersionedVariable lhsVar(varName, loc, VAR_POINTER, lhsVer);
            //VariableSMT *lhsForm = new VariableSMT();
            std::shared_ptr<VariableSMT> lhsForm(new VariableSMT(lhsVar));
            state.formulae.push_back(lhsForm);

            PtrCounter &rhsCnt = ctx.variables[ rhsName ];
            int rhsVer = rhsCnt.count;
            VersionedVariable rhsVar(rhsName, loc, VAR_POINTER, rhsVer);
            //VariableSMT *rhsForm = new VariableSMT();
            std::shared_ptr<VariableSMT> rhsForm(new VariableSMT(rhsVar));
            state.formulae.push_back(rhsForm);
            lhsCnt.meta = rhsCnt.meta;

            //shared_ptr<VariableSMT> leftForm(new VariableSMT);
            //leftForm->Var(lhsVar);
            //state.formulae.push_back(leftForm);
            std::shared_ptr<FormulaSMT> bs(new BinarySMT(lhsVar, rhsVar, EqualSMT, false));
            state.formulae.push_back(bs);

            ctx.fsm.AddStateToLeaves(state, ctx.fairPred);
        }

    private:
        std::string varName;
        std::string rhsName;
        std::string loc;
    };


    class VarAssigmentNewStatement : public Statement
    {
    public:
        VarAssigmentNewStatement( const std::string &varName, bool isArray, const std::string &loc )
                : varName(varName), isArray(isArray), loc(loc)
        {
        }

        void process( ProcessCtx &ctx ) override
        {
            PtrCounter &lhsCnt = ctx.variables[ varName ];
            int lhsVer = ++(lhsCnt.count);

            StateFSM state;
            VersionedVariable vv( varName, loc, VAR_POINTER, lhsCnt.count);
            if( isArray )
            {
                vv.MetaType(VAR_ARRAY_POINTER);
                state.allocArrays.push_back(vv);
                lhsCnt.meta = VAR_ARRAY_POINTER;
            }
            else
            {
                state.allocPointers.push_back(vv);
                lhsCnt.meta = VAR_POINTER;
            }
            //Отметить new.
            ctx.allocated.push_back(vv);

            std::shared_ptr<VariableSMT> vvForm(new VariableSMT(vv));
            state.formulae.push_back(vvForm);

            ctx.fsm.AddStateToLeaves(state, ctx.fairPred);
        }

    private:
        std::string varName;
        bool isArray;
        std::string loc;
    };


    class ReturnStatement : public Statement
    {
    public:
        ReturnStatement( const std::string &returnVarName )
                : returnVarName(returnVarName)
        {
            static int a = 123;
            a++;
        }

        void process( ProcessCtx &ctx ) override
        {
            if( !returnVarName.empty())
            {
                std::set<std::string> tmp;
                tmp.insert(returnVarName);
                ctx.fsm.SetReturnVarName(tmp);
            }

            ctx.fsm.ProcessReturnNone();

            bool isAllocFoo = ctx.fsm.IsAllocReturns();
            if( isAllocFoo )
            {
                allocatedFunctions.insert(ctx.fsm.FunctionName());
            }

            ctx.fsm.ClearReturnVarName();
        }

    private:
        std::string returnVarName;
    };


    class Function
    {
    public:
//        using FooPtr = std::shared_ptr<Function>;

        void process()
        {
            // already proccessed
            if( processedFunctions.find(name) != processedFunctions.end())
            {
                return;
            }

            // process all callee
            for( const auto &c : callee )
            {
                if( allFunctions.find(c) == allFunctions.end())
                {
                    // KABUM
                    continue;
//                __debugbreak();
                }

                // prevent recursion
                if( name != allFunctions[ c ]->name )
                {
                    allFunctions[ c ]->process();
                }
            }

            // process
            ProcessCtx ctx;
            ctx.fsm.FunctionName(name);
            static int x = 123;
            if( name == "SCR_ScreenShot_f_1" )
            {
                ++x;
            }
///         ?????
            statement->process(ctx);

            if( !returnName.empty())
            {
                ctx.fsm.SetReturnVarName(returnName);
            }
            ctx.fsm.ProcessReturnNone();

            processedFunctions.insert(name);
        }

    public:
        CompoundStatement *statement = nullptr;
        std::vector<std::string> callee;

        std::string name;
        std::set<std::string> returnName;
    };

// TODO: выяснить зачем этот класс
    class Module
    {
    public:

        Function *addFoo()
        {
            extFunctions.push_back(new Function());
            return extFunctions.back();
        }

    private:
        std::vector<Function *> extFunctions;
        // TODO:
        std::vector<Function> staticFunctions;
    };


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