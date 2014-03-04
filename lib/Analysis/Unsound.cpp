#define DEBUG_TYPE "Project2Unsound"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "Reaching.cpp"
#include <vector>
using namespace llvm;

using std::vector;

namespace {
  // Hello - The first implementation, without getAnalysisUsage.
  struct Unsound : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Unsound() : FunctionPass(ID) {}

	virtual bool runOnFunction(Function &F)
	{
		Reaching &reaching = getAnalysis<Reaching>();
		errs() << "Test\n";
		return false;
	}

	void getAnalysisUsage(AnalysisUsage &AU) const
	{
		AU.addRequired<Reaching>();
		AU.addPreserved<Reaching>();
	}
  };
}
char Reaching::ID = 0;
static RegisterPass<Reaching> X("reaching", "CSE6142 Project 2 Reaching Pass", false, true);

char Unsound::ID = 0;
static RegisterPass<Unsound> Y("project2Unsound", "Finds unsound variable uses for project 2");
