#ifndef ASTWalker_h
#define ASTWalker_h


#include <clang/Lex/HeaderSearchOptions.h>
#include <clang/AST/Stmt.h>


class ESCAASTConsumer;


class ASTWalker
{
public:
    ASTWalker();

    ~ASTWalker();

    bool WalkAST( const std::string &path );

    void DumpStmt( clang::Stmt *s );

    /// @brief Добавляет файлы где искать include библиотеки используемые в проекте
    /// @param paths - список директорий, содержащих библиотеки
    void SetIncludeDirectories( const std::vector<std::string> &paths );

protected:
    std::shared_ptr<clang::HeaderSearchOptions> headerSearchOptions;
    std::unique_ptr<clang::ASTContext> astContext;
    std::unique_ptr<ESCAASTConsumer> astConsumer;
};

#endif
