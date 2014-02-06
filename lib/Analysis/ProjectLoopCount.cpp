#define DEBUG_TYPE "projectlooppassd"
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

STATISTIC(loopAllTot, "Total number of loops in any function");
STATISTIC(loopTopTot, "Total number of top level loops in any function");
STATISTIC(loopExitTot, "Total number of loop exits in any function");

namespace
{
	struct ProjectLoopPass : public FunctionPass
	{
		static unsigned int loopTot;
		static unsigned int loopBlockTot;
		static unsigned int blockTot;
		static unsigned int blockDomTot;
		static unsigned int domTot;
		static char ID;
		ProjectLoopPass() : FunctionPass(ID){}

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

			//Get loop info
			LoopInfo &LI = getAnalysis<LoopInfo>();
			unsigned int allLoops = 0;
			unsigned int allLoopExits = 0;
			for(LoopInfo::iterator itr = LI.begin(); itr != LI.end();)
			{
				LoopBase<BasicBlock, Loop> *loopBase = *itr;

				//Count sub loops
				traverseSubLoops(loopBase, &allLoops, &allLoopExits);

				//This is a top lvl loop
				loopTopTot++;

				itr++;
			}
			loopExitTot += allLoopExits;
			loopAllTot += allLoops;

			return false;
		}

		void getAnalysisUsage(AnalysisUsage &AU) const
		{
			AU.addRequired<LoopInfo>();
			AU.addPreserved<LoopInfo>();
		}
	};

	unsigned int ProjectLoopPass::loopTot = 0;
	char ProjectLoopPass::ID = 0;
	static RegisterPass<ProjectLoopPass> X("projectlooppass", "CSE6142 Project 1 loop pass (3.2)", false, true);
}
