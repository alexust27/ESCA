/// @file AnalyzeProcess.h
///
/// @brief Класс для запуска анализатора на основе наших состояний
///
/// @author alexust27
/// Contact: ustinov1998s@gmail.com
///
#ifndef ESCA_PROCESSCTX_H
#define ESCA_PROCESSCTX_H

#include "Context.h"
#include "ProcessContext.h"

using namespace Target;

class AnalyzeProcess
{

public:
    AnalyzeProcess();

    void StartAnalyze();

private:

    void ProcessFunction( Target::Function *function );

    void ProcessStatement( Statement *stmt );

    void ProcessCompound( Target::CompoundStatement *statement );

    void ProcessVarAssigmentFromFoo( VarAssigmentFromFooStatement *statement );

    void ProcessVarAssigmentFromPointer( VarAssigmentFromPointerStatement *statement );

    void ProcessVarAssigmentNew( VarAssigmentNewStatement *statement );

    void ProcessDelete( DeleteStatement *statement );

    void ProcessIF( IfStatement *statement );

    void ProcessReturn( ReturnStatement *statement );


private:

    /// @brief Множество проанализированных функций
    std::set<std::string> processedFunctions;

    /// @brief Множество функции которые возвращают указатель на выделенную память
    std::set<std::string> allocatedFunctions;

    /// @brief Уникальный контекст для одной функции
    std::unique_ptr<ProcessContext> processContext;
};


#endif //ESCA_PROCESSCTX_H
