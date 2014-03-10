//#ifdef NOTDEF
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

		set<Value*> assigned;
		set<Value*> used;

		for(Function::iterator itr = F.begin(); itr != F.end(); itr++)
		{
			set<Value*> localAssigns;

			for(BasicBlock::iterator blockItr = itr->begin(); blockItr != itr->end(); blockItr++)
			{
				if((!dyn_cast<Instruction>(&*blockItr) && !dyn_cast<Operator>(&*blockItr))
					|| dyn_cast<CallInst>(&*blockItr))
				{
					localAssigns.insert(&*blockItr);
					continue;
				}
				else if(dyn_cast<AllocaInst>(&*blockItr))
				{
					assigned.insert(&*blockItr);
					continue;
				}
				else if(dyn_cast<StoreInst>(&*blockItr))
				{
					Value* storeOper = blockItr->getOperand(1);
					localAssigns.insert(storeOper);
					continue;
				}

				localAssigns.insert(&*blockItr);

				int numOper = blockItr->getNumOperands();
				for(int i = 0; i < numOper; i++)
				{
					Value* oper = blockItr->getOperand(i);

					used.insert(oper);

					//We don't care about jumps to basic block or the use of constants
					if(dyn_cast<Constant>(oper))continue;
					if(dyn_cast<BasicBlock>(oper))continue;

					if(localAssigns.find(oper) == localAssigns.end() && reaching.reaches(oper, &*itr).size() == 0)
					{
						errs() << "Unsafe: " << *blockItr << "\n";
						errs() << "Location Line: " << blockItr->getDebugLoc().getLine() << "\n";
					}
				}
			}

		}

		//See which variables were assigned, but never read
		for(set<Value*>::iterator itr = assigned.begin(); itr != assigned.end(); itr++)
		{
			Instruction* val = dyn_cast<Instruction>(*itr);
			if(used.find(val) == used.end())
			{
				errs() << "Variable Unused: " << val->getName();
				errs() << ". In function: " << val->getParent()->getParent()->getName() << "\n";
				//errs() << ". Defined at line: " << val->getDebugLoc().getLine() << "\n";
			}
		}

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
//#endif
