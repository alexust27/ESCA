#ifndef ASTWalker_h
#define ASTWalker_h


#include <clang/Frontend/TextDiagnosticPrinter.h>
#include "clang/Frontend/CompilerInstance.h"
#include <clang/Lex/HeaderSearchOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include "ESCAASTConsumer.h"

class ASTWalker
{
public:
    explicit ASTWalker( const std::vector<std::string> &paths );

//    ~ASTWalker();

    /// @brief Метод проходим по файлу и сохранает информацию в контекст
    /// @param fileName - файл для анализа
    /// @return true - если удалось проанализировать файл, false - если возникла какая-либо ошибка во время анализа
    bool WalkAST( const std::string &fileName );

    /// @brief Метод проходит по нескольким файлам и сохранает информацию в контекст
    /// @param files - список файлов для анализа
    void WalkAST( const std::vector<std::string> &files );


#ifdef DEBUG

    void DumpStmt( clang::Stmt *s );

#endif

protected:

    /// @brief Добавляет файлы где искать include библиотеки используемые в проекте
    /// @param paths - список директорий, содержащих библиотеки
    void SetIncludeDirectories( const std::vector<std::string> &paths );


    std::shared_ptr<clang::HeaderSearchOptions> headerSearchOptions;
    std::unique_ptr<clang::ASTContext> astContext;
    std::unique_ptr<ESCAASTConsumer> astConsumer;

private:
    clang::DiagnosticsEngine *pDiagnosticsEngine;
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> pDiagIDs;
    clang::FileSystemOptions fileSystemOptions = clang::FileSystemOptions();
    clang::CompilerInstance compInst;
    std::shared_ptr<clang::PreprocessorOptions> pOpts;
    llvm::Triple triple;
    clang::PreprocessorOptions ppopts;
    std::shared_ptr<clang::TargetOptions> targetOptions;
    clang::DiagnosticOptions diagnosticOptions;
    clang::LangOptions languageOptions;
    std::unique_ptr<clang::SourceManager> sourceManager;
    std::unique_ptr<clang::FileManager> fileManager;
    const clang::TargetInfo *pTargetInfo;
    clang::Preprocessor *preprocessor;
    clang::FrontendOptions frontendOptions;
    clang::RawPCHContainerReader containerReader;
    std::shared_ptr<clang::InputKind> ik;
    std::shared_ptr<clang::HeaderSearch> headerSearch;
    std::unique_ptr<clang::TextDiagnosticPrinter> pTextDiagnosticPrinter;

    void makeContext();
};

#endif
