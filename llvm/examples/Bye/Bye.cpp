#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct Bye : PassInfoMixin<Bye> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    errs() << "Bye: ";
    errs().write_escaped(F.getName()) << '\n';

    BasicBlock &entryBlock = F.getEntryBlock();
    IRBuilder<> frontBuilder(&entryBlock.front());
    frontBuilder.CreateCall(FunctionCallee(F.getParent()->getOrInsertFunction("instrument_start", Type::getVoidTy(F.getContext()))));

    BasicBlock &backBlock = F.back();
    IRBuilder<> backBuilder(&backBlock.back());
    backBuilder.CreateCall(FunctionCallee(F.getParent()->getOrInsertFunction("instrument_end", Type::getVoidTy(F.getContext()))));

    return PreservedAnalyses::all();
  }
};

} // namespace

/* New PM Registration */
llvm::PassPluginLibraryInfo getByePluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Bye", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerVectorizerStartEPCallback(
                [](llvm::FunctionPassManager &PM, OptimizationLevel Level) {
                  PM.addPass(Bye());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, llvm::FunctionPassManager &PM,
                   ArrayRef<llvm::PassBuilder::PipelineElement>) {
                  if (Name == "goodbye") {
                    PM.addPass(Bye());
                    return true;
                  }
                  return false;
                });
          }};
}

#ifndef LLVM_BYE_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getByePluginInfo();
}
#endif
