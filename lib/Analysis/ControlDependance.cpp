#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CFG.h"
#include <vector>

using namespace llvm;
using std::vector;

namespace
{
	struct ControlDependance : public FunctionPass
	{
		static char ID;
		ControlDependance() : FunctionPass(ID){}

		virtual bool runOnFunction(Function &F)
		{
			//Get dominator info
			PostDominatorTree &tree = getAnalysis<PostDominatorTree>();
			//DomTreeNode *rootNode = tree.getRootNode();

			vector<BasicBlock*> allBlocks;

			//Module::FunctionListType &functions = M.getFunctionList();

			//Go through each function and get all the blocks
			//for(Module::FunctionListType::iterator itr = functions.begin(); itr != functions.end(); itr++)
			//{
				Function::BasicBlockListType &blocks = F.getBasicBlockList();
				for(Function::BasicBlockListType::iterator blockItr = blocks.begin();
					blockItr != blocks.end(); blockItr++)
				{
					allBlocks.push_back(&*blockItr);
				}

			//}

			//Loop through all combinations of (i, j) looking for the post-dominance condiations

			int numBlocks = allBlocks.size();
			for(int i = 0; i < numBlocks; i++)
			{
				BasicBlock* blockI = allBlocks.at(i);
				for(int j = 0; j < numBlocks; j++)
				{
					BasicBlock* blockJ = allBlocks.at(j);

					//i should not be post-dominated by j
					if(tree.dominates(blockJ, blockI)) continue;

					//Get children of i
					TerminatorInst *term = blockI->getTerminator();
					int numSucc = term->getNumSuccessors();

					for(int k = 0; k < numSucc; k++)
					{
						BasicBlock *child = term->getSuccessor(k);
						if(tree.dominates(blockJ, child))
						{
							errs() << blockI->getName() << " -> ";
							errs() << blockJ->getName() << "\n";
						}
						
					}
				}
			}

			return false;

		}

		void getAnalysisUsage(AnalysisUsage &AU) const
		{
			AU.addRequired<PostDominatorTree>();
			AU.addPreserved<PostDominatorTree>();
		}
	};

	char ControlDependance::ID = 0;
	static RegisterPass<ControlDependance> X("controldependance", "CSE6142 Project 1 ControlDependance Pass", false, true);
}
