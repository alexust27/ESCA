#include "Sort.h"

#include <functional>
#include <unordered_set>

#include <llvm/IR/Instructions.h>

std::unordered_set<const llvm::Function *> get_called_functions(const llvm::Function * func)
{
    std::unordered_set<const llvm::Function *> res;
    for (auto const & bb : *func)
    {
        for (auto const & instr : bb)
        {
            if (auto call = static_cast<llvm::CallInst const *>(&instr))
            {
                res.insert(call->getCalledFunction());
            }
        }
    }

    return res;
}

std::vector<const llvm::Function *> sort_functions(std::vector<const llvm::Function *> const & functions)
{
    std::vector<const llvm::Function *> res;
    std::unordered_set<const llvm::Function *> visited;
    std::function<void(const llvm::Function *)> dfs = [&dfs, &visited, &res](const llvm::Function * f)
    {
        if (visited.count(f))
            return;

        visited.insert(f);
        for (const llvm::Function * called : get_called_functions(f))
        {
            dfs(called);
        }

        res.push_back(f);
    };

    for (const llvm::Function * f : functions)
    {
        dfs(f);
    }

    return res;
}
