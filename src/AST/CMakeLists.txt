include_directories(../utils)
include_directories(../target)

add_library(ast SHARED
        ASTWalker.cpp
#        ESCAASTConsumer.cpp
        ESCAASTVisitor.cpp
        )
