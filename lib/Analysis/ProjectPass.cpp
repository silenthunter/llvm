#define DEBUG_TYPE "projectpassd"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Dominators.h"
#include <set>

using namespace llvm;
using std::vector;
using std::set;

STATISTIC(blockAvg, "Average number of blocks in functions");
STATISTIC(blockMin, "Min number of blocks in functions");
STATISTIC(blockMax, "Max number of blocks in functions");
STATISTIC(funcCnt, "Number of functions called");
STATISTIC(maxCFG, "Max CFG edges seen in a function");
STATISTIC(minCFG, "Min CFG edges seen in a function");
STATISTIC(avgCFG, "Avg CFG edges seen in a function");
STATISTIC(loopMin, "Min loop count seen in a function");
STATISTIC(loopMax, "Max loop count seen in a function");
STATISTIC(loopAvg, "Avg loop count seen in a function");
STATISTIC(loopBlockMin, "Min blocks in a loop count seen in a function");
STATISTIC(loopBlockMax, "Max blocks in a loop count seen in a function");
STATISTIC(loopBlockAvg, "Avg blocks in a loop count seen in a function");
STATISTIC(dominatorAvg, "Avg dominator count seen in a function");
STATISTIC(loopAllTot, "Total number of loops in any function");
STATISTIC(loopTopTot, "Total number of top level loops in any function");
STATISTIC(loopExitTot, "Total number of loop exits in any function");

namespace
{
	struct ProjectPass : public FunctionPass
	{
		static unsigned int loopTot;
		static unsigned int loopBlockTot;
		static unsigned int blockTot;
		static unsigned int blockDomTot;
		static unsigned int domTot;
		static char ID;
		ProjectPass() : FunctionPass(ID){}

		void traverseDominators(DominatorTree *tree, BasicBlock *block, DomTreeNode* node, unsigned int* count)
		{
			//Reached a branch that does not dominate our block
			if(node == NULL || !tree->dominates(node->getBlock(), block)) return;

			++(*count);

			std::vector<DomTreeNodeBase<BasicBlock>*> children = node->getChildren();
			for(std::vector<DomTreeNodeBase<BasicBlock>*>::iterator itr = children.begin(); itr != children.end(); itr++)
			{
				traverseDominators(tree, block, *itr, count);
			}

		}

		void traverseSubLoops(LoopBase<BasicBlock, Loop> *loop, unsigned int *count, unsigned int *exits)
		{
			(*count)++;

			//Look for loop exits
			SmallVector<BasicBlock*, 128> exitVec;
			loop->getExitingBlocks(exitVec);
			*exits += exitVec.size();

			//Recurse through sub-loops
			vector<Loop*> subLoops = loop->getSubLoops();

			for(vector<Loop*>::iterator itr = subLoops.begin(); itr != subLoops.end(); itr++)
			{
				traverseSubLoops(*itr, count, exits);
			}
		}

		virtual bool runOnFunction(Function &F)
		{
			++funcCnt;

			//Get dominator info
			DominatorTree &tree = getAnalysis<DominatorTree>();
			DomTreeNode *rootNode = tree.getRootNode();

			//Get loop info
			LoopInfo &LI = getAnalysis<LoopInfo>();
			unsigned int loopBlocks = 0;
			unsigned int allLoops = 0;
			unsigned int allLoopExits = 0;
			for(LoopInfo::iterator itr = LI.begin(); itr != LI.end();)
			{
				LoopBase<BasicBlock, Loop> *loopBase = *itr;
				loopBlocks += loopBase->getBlocks().size();

				//Count sub loops
				traverseSubLoops(loopBase, &allLoops, &allLoopExits);

				//This is a top lvl loop
				loopTopTot++;

				itr++;
			}
			loopExitTot += allLoopExits;
			loopAllTot += allLoops;
			loopBlockTot += loopBlocks;
			if(loopBlocks > loopBlockMax) loopBlockMax = loopBlocks; 
			if(loopBlocks < loopBlockMin || loopBlockMin == 0) loopBlockMin = loopBlocks; 
			loopBlockAvg = loopBlockTot / funcCnt;



			//Get block info
			unsigned int blockCnt = F.getBasicBlockList().size();
			if(blockCnt > blockMax) blockMax = blockCnt;
			if(blockCnt < blockMin || blockMin == 0) blockMin = blockCnt;
			blockTot += blockCnt;
			blockAvg = blockTot / funcCnt;

			//Get number of internal edges and loops
			set<BasicBlock*> loopHead;
			int loopSize = 0;
			unsigned int edges = 0;
			Function::iterator itr = F.begin();
			while(itr != F.end())
			{
				blockDomTot++;
				unsigned int numSucc = itr->getTerminator()->getNumSuccessors();

				//Get backedges
				for(int i = 0; i < numSucc; i++)
				{
					BasicBlock *succBlock = itr->getTerminator()->getSuccessor(i);
					//This is a backedge!
					if(tree.dominates(succBlock, itr))
					{
						loopHead.insert(succBlock);
					}
				}

				edges += numSucc;
				traverseDominators(&tree, itr, tree.getRootNode(), &domTot);
				itr++;
			}

			loopSize = loopHead.size();

			//Update loop stats
			if(loopSize > loopMax) loopMax = loopSize;
			if(loopSize < loopMin || loopMin == 0) loopMin = loopSize;
			loopTot += loopSize;
			loopAvg = loopTot / funcCnt;

			//Update edges
			if(edges > maxCFG) maxCFG = edges;
			if(edges < minCFG || minCFG == 0) minCFG = edges;
			dominatorAvg = domTot / blockDomTot;
			return false;
		}

		void getAnalysisUsage(AnalysisUsage &AU) const
		{
			AU.addRequired<LoopInfo>();
			AU.addPreserved<LoopInfo>();
			AU.addRequired<DominatorTree>();
			AU.addPreserved<DominatorTree>();
		}
	};

	unsigned int ProjectPass::loopTot = 0;
	unsigned int ProjectPass::loopBlockTot = 0;
	unsigned int ProjectPass::blockTot = 0;
	unsigned int ProjectPass::blockDomTot = 0;;
	unsigned int ProjectPass::domTot = 0;;
	char ProjectPass::ID = 0;
	static RegisterPass<ProjectPass> X("projectpass", "CSE6142 Project 1 Pass", false, true);
}
