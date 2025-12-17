/* DerivedInductionVar.cpp
 *
 * This pass detects derived induction variables using ScalarEvolution.
 *
 * Compatible with New Pass Manager
 */

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>

using namespace llvm;

namespace {

class DerivedInductionVar : public PassInfoMixin<DerivedInductionVar> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &LI = AM.getResult<LoopAnalysis>(F);
    auto &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
    const DataLayout &DL = F.getParent()->getDataLayout();

    SCEVExpander Expander(SE, DL, "derived-iv");

    bool Changed = false;

    for (Loop *L : LI.getLoopsInPreorder()) {
      BasicBlock *Header = L->getHeader();
      if (!Header)
        continue;

      auto IsLoopSkeleton = [&](Instruction *I) -> bool {
        for (User *U : I->users()) {
          if (PHINode *PN = dyn_cast<PHINode>(U)) {
            if (PN->getParent() == Header)
              return true;
          }
        }
        return false;
      };

      PHINode *canonicalIV = L->getCanonicalInductionVariable();
      SmallVector<Instruction *, 8> deadInsts;

      /* iterate over all instructions in the loop body */
      for (BasicBlock *BB : L->getBlocks()) {
        for (Instruction &I : *BB) {

          /* skip terminators (branches) */
          if (I.isTerminator())
            continue;

          /* skip the Canonical IV (the counter itself) */
          if (&I == canonicalIV)
            continue;

          /* skip instructions feeding the Loop Header PHIs (Backedge updates)
           */
          if (IsLoopSkeleton(&I))
            continue;

          /* only care about integer or pointer computations */
          if (!I.getType()->isIntOrPtrTy())
            continue;

          /* ask ScalarEvolution what this value is */
          if (!SE.isSCEVable(I.getType()))
            continue;

          const SCEV *S = SE.getSCEV(&I);

          /* detect affine AddRec expressions */
          if (auto *AR = dyn_cast<SCEVAddRecExpr>(S)) {

            /* make sure this recurrence belongs to the current loop and is
             * affine */
            if (AR->getLoop() != L || !AR->isAffine())
              continue;

            errs() << "Analyzing loop in function " << F.getName() << ":\n";
            errs() << "  Found Derived IV: " << I.getName() << " = " << *AR
                   << "\n";

            /* determine insertion point */
            Instruction *InsertPt = &I;
            if (isa<PHINode>(I)) {
              InsertPt = Header->getFirstNonPHI();
            }

            if (Expander.isSafeToExpand(AR)) {
              Value *NewVal = Expander.expandCodeFor(AR, I.getType(), InsertPt);

              if (NewVal != &I) {
                /* IMPORTANT: forget value to clean up SCEV cache */
                SE.forgetValue(&I);

                I.replaceAllUsesWith(NewVal);
                deadInsts.push_back(&I);
                Changed = true;
                errs() << "  -> Eliminated and replaced with expanded SCEV.\n";
              }
            }
          }
        }
      }

      /* cleanup dead instructions */
      for (Instruction *Dead : deadInsts) {
        if (Dead->use_empty()) {
          SE.forgetValue(Dead);
          Dead->eraseFromParent();
        }
      }
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};
} // namespace

// Register the pass
llvm::PassPluginLibraryInfo getDerivedInductionVarPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "DerivedInductionVar", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "derived-iv") {
                    FPM.addPass(DerivedInductionVar());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getDerivedInductionVarPluginInfo();
}
