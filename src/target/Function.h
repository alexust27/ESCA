#include "Statement.h"

namespace Target {
    class Function;
}

extern std::set<std::string> allocatedFunctions;
extern std::map<std::string, Target::Function *> allFunctions;
extern std::set<std::string> processedFunctions;

namespace Target {
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
}
