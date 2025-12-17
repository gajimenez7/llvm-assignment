/* SimpleLICM.cpp
 *
 * This pass hoists loop-invariant code before the loop when it is safe to do
 * so.
 *
 * Compatible with New Pass Manage
 */

#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

struct SimpleLICM : public PassInfoMixin<SimpleLICM> {
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &) {

    errs() << "\n\n -- Running SimpleLICM on loop with " << L.getBlocks().size()
           << " blocks -- \n\n";

    DominatorTree &DT = AR.DT;

    BasicBlock *Preheader = L.getLoopPreheader();
    if (!Preheader) {
      errs() << "No preheader, skipping loop\n";
      return PreservedAnalyses::all();
    }

    SmallPtrSet<Instruction *, 8> InvariantSet;
    bool Change = true;

    // Worklist algorithm to identify loop invariant instructions
    /*************************************/
    while (Change) {
      Change = false;
      /* basic blocks from loop */
      for (BasicBlock *BB : L.getBlocks()) {
        /* instructions in basic block */
        for (Instruction &I : *BB) {

          // errs() << "Instruction:\n\t>" << I << "\n";
          /* skip if current instruction is in the Invariant Set */
          if (InvariantSet.contains(&I)) {
            continue;
          }

          /* check that there are no unsafe operations */
          if (isa<PHINode>(I) || I.mayReadFromMemory() ||
              I.mayWriteToMemory() || I.isVolatile() ||
              I.isSpecialTerminator()) {
            continue;
          }

          bool isInvariant = true;

          /* iterate over instruction operands */
          for (Use &op : I.operands()) {
            // errs() << "Operation:\n\t>" << *op << "\n";

            /* is instruction constant or already invariant */
            if (Instruction *opI = dyn_cast<Instruction>(op)) {
              if (L.contains(opI) && !InvariantSet.contains(opI)) {
                isInvariant = false;
                break;
              }
            }
          }

          if (isInvariant && isSafeToSpeculativelyExecute(&I)) {
            InvariantSet.insert(&I);
            Change = true;
            errs() << "!Marked Invariant: \n\t>" << I << "!\n";
          }
        }
      }
    }
    bool Changed = false;
    /*************************************/

    /*
     * update to hoisting behavior, ensuring topological order to
     * prevent memory issues where order is not kept
     */
    SmallVector<Instruction *, 8> ToHoist;
    // Actually hoist the instructions
    for (BasicBlock *BB : L.getBlocks()) {
      for (Instruction &I : *BB) {

        if (InvariantSet.contains(&I)) {
          ToHoist.push_back(&I);
        }
      }
    }

    for (Instruction *I : ToHoist) {
      /* we already check if save to speculatively execute */
      // if (dominatesAllLoopExits(I, &L, DT)) {
      errs() << "Hoisting: " << *I << "\n";
      I->moveBefore(Preheader->getTerminator());
      Changed = true;
      //}
    }

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

  bool dominatesAllLoopExits(Instruction *I, Loop *L, DominatorTree &DT) {
    SmallVector<BasicBlock *, 8> ExitBlocks;
    L->getExitBlocks(ExitBlocks);
    for (BasicBlock *EB : ExitBlocks) {
      if (!DT.dominates(I, EB))
        return false;
    }
    return true;
  }
};

llvm::PassPluginLibraryInfo getSimpleLICMPluginInfo() {
  errs() << "SimpleLICM plugin: getSimpleLICMPluginInfo() called\n";
  return {LLVM_PLUGIN_API_VERSION, "simple-licm", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, LoopPassManager &LPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "simple-licm") {
                    LPM.addPass(SimpleLICM());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  errs() << "SimpleLICM plugin: llvmGetPassPluginInfo() called\n";
  return getSimpleLICMPluginInfo();
}
