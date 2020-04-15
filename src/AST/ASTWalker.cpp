#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Builtins.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Parse/ParseAST.h>

#include <clang/AST/ASTContext.h>

#include <llvm/Support/Host.h>
#include <llvm/IR/Function.h>

#include "ASTWalker.h"
#include "ESCAASTConsumer.h"

ASTWalker::ASTWalker()
        : headerSearchOptions(new clang::HeaderSearchOptions()), astConsumer(new ESCAASTConsumer)
{
}

ASTWalker::~ASTWalker()
{
//    delete astConsumer;
//    delete astContext;
}

void ASTWalker::SetIncludeDirectories( const std::vector<std::string> &paths )
{
    for( const auto &path : paths )
    {
        // TODO: разобраться почему идет проверка и вывод варнингов о системных либах
        //      (исправлено перемещением потока stdout в файл)
        headerSearchOptions->AddPath(path, clang::frontend::Angled, false, false);
    }
}

void ASTWalker::WalkAST( const std::string &path )
{
    clang::DiagnosticOptions diagnosticOptions;
    auto *pTextDiagnosticPrinter = new clang::TextDiagnosticPrinter(llvm::outs(), &diagnosticOptions);
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> pDiagIDs;
    auto *pDiagnosticsEngine = new clang::DiagnosticsEngine(pDiagIDs,
                                                            &diagnosticOptions, pTextDiagnosticPrinter);

    clang::LangOptions languageOptions;
    clang::FileSystemOptions fileSystemOptions = clang::FileSystemOptions();
    clang::FileManager fileManager(fileSystemOptions);

    clang::SourceManager sourceManager(*pDiagnosticsEngine, fileManager);

    clang::InputKind ik(clang::Language::CXX, clang::InputKind::Source, false);
    llvm::Triple triple;
    clang::PreprocessorOptions ppopts;
    clang::CompilerInvocation::setLangDefaults(languageOptions, ik, triple, ppopts);
    languageOptions.ImplicitInt = 1;


    auto targetOptions = std::make_shared<clang::TargetOptions>();
    targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
    clang::TargetInfo *pTargetInfo =
            clang::TargetInfo::CreateTargetInfo(
                    *pDiagnosticsEngine,
                    targetOptions);

    clang::HeaderSearch headerSearch(headerSearchOptions,
                                     sourceManager,
                                     *pDiagnosticsEngine,
                                     languageOptions,
                                     pTargetInfo);

    clang::CompilerInstance compInst;

    auto pOpts = std::make_shared<clang::PreprocessorOptions>();
    clang::Preprocessor preprocessor(
            pOpts,
            *pDiagnosticsEngine,
            languageOptions,
            sourceManager,
            headerSearch,
            compInst);

    preprocessor.Initialize(*pTargetInfo);

    clang::FrontendOptions frontendOptions;
    clang::RawPCHContainerReader containerReader;

    clang::ApplyHeaderSearchOptions(
            headerSearch,
            *headerSearchOptions,
            languageOptions,
            triple
    );

    clang::InitializePreprocessor(
            preprocessor,
            *pOpts,
            containerReader,
            frontendOptions);


    llvm::ErrorOr<const clang::FileEntry *> pFile = fileManager.getFile(path);
//    clang::SourceLocation sourceLocation;
    auto mainID = sourceManager.getOrCreateFileID(pFile.get(), clang::SrcMgr::C_System);
    sourceManager.setMainFileID(mainID);
    const clang::TargetInfo &targetInfo = *pTargetInfo;

    clang::IdentifierTable identifierTable(languageOptions);
    clang::SelectorTable selectorTable;

    clang::Builtin::Context builtinContext;
    builtinContext.InitializeTarget(targetInfo, nullptr);

    astContext = std::make_unique<clang::ASTContext>(languageOptions, sourceManager, identifierTable, selectorTable,
                                                     builtinContext);
    astContext->InitBuiltinTypes(targetInfo);
//    astConsumer->Initialize(*astContext);
    {
        astConsumer->SetPath(path);
        pTextDiagnosticPrinter->BeginSourceFile(languageOptions, &preprocessor);
        clang::ParseAST(preprocessor, static_cast<clang::ASTConsumer *>(astConsumer.get()), *astContext);
        pTextDiagnosticPrinter->EndSourceFile();
    }
}

void ASTWalker::DumpStmt( clang::Stmt *s )
{
    s->dump(llvm::errs(), astContext->getSourceManager());
    s->dump();
}
