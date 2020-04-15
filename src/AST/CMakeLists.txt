include_directories(../utils)

add_library(ast SHARED
        ASTWalker.cpp
#        ESCAASTConsumer.cpp
        ESCAASTVisitor.cpp
        )
