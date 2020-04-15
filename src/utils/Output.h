#pragma once

//#define DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define Cout llvm::errs()
#else
#define Cout llvm::nulls()
#endif