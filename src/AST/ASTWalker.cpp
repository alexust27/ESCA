#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Builtins.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Parse/ParseAST.h>

#include <clang/AST/ASTContext.h>

#include <llvm/Support/Host.h>
#include <llvm/IR/Function.h>
#include <iostream>

#include "ASTWalker.h"

ASTWalker::ASTWalker( const std::vector<std::string> &paths )
        : headerSearchOptions(new clang::HeaderSearchOptions()), astConsumer(new ESCAASTConsumer())
{
    CommonStorage::Instance().SetIncludeDirs(paths);
    SetIncludeDirectories(paths);

    fileManager = std::make_unique<clang::FileManager>(fileSystemOptions);
    ik = std::make_shared<clang::InputKind>(clang::Language::CXX, clang::InputKind::Source, false);
    targetOptions = std::make_shared<clang::TargetOptions>();
    targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
    clang::CompilerInvocation::setLangDefaults(languageOptions, *ik, triple, ppopts);
    languageOptions.ImplicitInt = 1;

}

//ASTWalker::~ASTWalker()
//{
//    pDiagIDs.reset();
//    pDiagnosticsEngine.reset();
//    pOpts.reset();
//    targetOptions.reset();
//    sourceManager.reset();
//    fileManager.reset();
//    delete pTargetInfo;
//    delete preprocessor;
//    ik.reset();
//    headerSearch.reset();
//    pTextDiagnosticPrinter.reset();
//}

void ASTWalker::makeContext()
{
#ifdef TEXT_DIAG1
    // TextDiagnosticPrinter анализирует файлы, чтобы не выводил весь анализ в stdout,
    // создаем файл куда и выводим всю информацию
    std::error_code ec;
    llvm::raw_fd_ostream dbg_inf("diagnostic_info.txt", ec);
    if( ec )
    {
        std::cerr << ec.message() << std::endl;
        return;
    }
    pTextDiagnosticPrinter = std::make_unique<clang::TextDiagnosticPrinter>(dbg_inf, &diagnosticOptions);
#else
    pTextDiagnosticPrinter = std::make_unique<clang::TextDiagnosticPrinter>(llvm::nulls(), &diagnosticOptions);
#endif
    pDiagnosticsEngine = new clang::DiagnosticsEngine(pDiagIDs, &diagnosticOptions, pTextDiagnosticPrinter.get());

    sourceManager = std::make_unique<clang::SourceManager>(*pDiagnosticsEngine, *fileManager);

    pTargetInfo = clang::TargetInfo::CreateTargetInfo(*pDiagnosticsEngine, targetOptions);

    headerSearch = std::make_shared<clang::HeaderSearch>(headerSearchOptions,
                                                         *sourceManager,
                                                         *pDiagnosticsEngine,
                                                         languageOptions,
                                                         pTargetInfo);


    pOpts = std::make_shared<clang::PreprocessorOptions>();
    preprocessor = new clang::Preprocessor(
            pOpts,
            *pDiagnosticsEngine,
            languageOptions,
            *sourceManager,
            *headerSearch,
            compInst
    );

    preprocessor->Initialize(*pTargetInfo);


    clang::ApplyHeaderSearchOptions(
            *headerSearch,
            *headerSearchOptions,
            languageOptions,
            triple
    );

    clang::InitializePreprocessor(
            *preprocessor,
            *pOpts,
            containerReader,
            frontendOptions
    );

}

bool ASTWalker::WalkAST( const std::string &fileName )
{
    llvm::ErrorOr<const clang::FileEntry *> pFile = fileManager->getFile(fileName);
    if( !pFile )
    {
        std::cerr << pFile.getError().message() << ", file: " << fileName << std::endl;
        return false;
    }

    if( !CommonStorage::Instance().AddAnalyzeFile(fileName))
    {
        return false;
    }
    makeContext();

    auto mainID = sourceManager->getOrCreateFileID(pFile.get(), clang::SrcMgr::C_User);
    sourceManager->setMainFileID(mainID);

    const clang::TargetInfo &targetInfo = *pTargetInfo;
    clang::IdentifierTable identifierTable(languageOptions);
    clang::SelectorTable selectorTable;
    clang::Builtin::Context builtinContext;
    builtinContext.InitializeTarget(targetInfo, nullptr);

    astContext = std::make_unique<clang::ASTContext>(languageOptions, *sourceManager, identifierTable, selectorTable,
                                                     builtinContext);
    astContext->InitBuiltinTypes(targetInfo);
//    astConsumer->Initialize(*astContext);
    pTextDiagnosticPrinter->BeginSourceFile(languageOptions, preprocessor);
    clang::ParseAST(*preprocessor, static_cast<clang::ASTConsumer *>(astConsumer.get()), *astContext);
    pTextDiagnosticPrinter->EndSourceFile();
    delete preprocessor;
    delete pTargetInfo;
//    delete pDiagnosticsEngine;
    return true;
}


void ASTWalker::SetIncludeDirectories( const std::vector<std::string> &paths )
{
    for( const auto &path : paths )
    {
        headerSearchOptions->AddPath(path, clang::frontend::System, false, false);
    }
}


void ASTWalker::WalkAST( const std::vector<std::string> &files )
{
    for( const auto &file : files )
    {
        try
        {
            if( !WalkAST(file))
            {
                std::cerr << "Failed to walk for file: " << file << std::endl;
            }
        }
        catch( std::exception &e )
        {
            std::cerr << "File : " << file << ". exception: " << e.what();
        }
        catch( ...)
        {
            std::cerr << "unhandled err" << std::endl;
        }
//        std::cout << "Start analyze for " << file << std::endl;
        std::cout << "Finish walk file: " << file << std::endl;
    }
    std::cout << "End walk." << std::endl;
    std::cout << "---------------------------------------" << std::endl;
}

#ifdef DEBUG

void ASTWalker::DumpStmt( clang::Stmt *s )
{
    s->dump(llvm::errs(), astContext->getSourceManager());
    s->dump();
}

#endif

