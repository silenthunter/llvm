#define DEBUG_TYPE "warshall_loop"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/CFG.h"

using namespace llvm;

STATISTIC(warshallLoop, "Number of loops found using the Warshall Algorithm");

namespace
{
	struct Warshall : public FunctionPass
	{
		static char ID;
		Warshall() : FunctionPass(ID){}

		virtual bool runOnFunction(Function &F)
		{
			//Get dominator info
			DominatorTree &tree = getAnalysis<DominatorTree>();

			//Get loop info
			LoopInfo &LI = getAnalysis<LoopInfo>();

			Function::BasicBlockListType &blocks = F.getBasicBlockList();
			for(Function::iterator itr = blocks.begin(); itr != blocks.end(); itr++)
			{
				for(Function::iterator itr2 = blocks.begin(); itr2 != blocks.end(); itr2++)
				{
					TerminatorInst *term = itr2->getTerminator();
					int succ = term->getNumSuccessors();
					for(int i = 0; i < succ; i++)
					{
						BasicBlock *succBlock = term->getSuccessor(i);
						if(succBlock == &(*itr) && isPotentiallyReachable(&(*itr), &(*itr2), &tree, &LI))
						{
							errs() << /*F.getName() << "." << */itr->getName()<<" -> ";
							errs() << /*F.getName() << "." << */itr2->getName()<<"\n";
							warshallLoop++;
						}
					}
					
				}
			}
		}

		void getAnalysisUsage(AnalysisUsage &AU) const
		{
			AU.addRequired<DominatorTree>();
			AU.addPreserved<DominatorTree>();
			AU.addRequired<LoopInfo>();
			AU.addPreserved<LoopInfo>();
		}
	};

	char Warshall::ID = 0;
	static RegisterPass<Warshall> X("warshall", "CSE6142 Project 1 Warshall Pass", false, true);
}
