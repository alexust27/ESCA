#include "Function.h"

namespace Target
{
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
    std::vector <Function> staticFunctions;
};
}