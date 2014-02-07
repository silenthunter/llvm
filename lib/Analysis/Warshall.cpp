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
#include <set>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

using namespace llvm;
using std::set;
using std::vector;
using std::string;

STATISTIC(warshallLoop, "Number of loops found using the Warshall Algorithm");

namespace
{
	struct Warshall : public FunctionPass
	{
		static char ID;
		Warshall() : FunctionPass(ID){}

		set<BasicBlock*> visited;
		set<string> cycles;

		bool mycompr(string a, string b)
		{
			if(a != NULL)
			return true;
			return false;
		}

		bool isReachable(BasicBlock *from, BasicBlock *to)
		{
			visited.clear();
			vector<string> path;
			bool retn = isReachableRecurse(from, to, &path);

			std::sort(path.begin(), path.end());

			//Create a hash of the cycle based on an alphabatized list of block names
			string hash = "";
			for(vector<string>::iterator itr = path.begin(); itr != path.end(); itr++)
				hash += *itr;

			if(cycles.find(hash) != cycles.end())
				return false;
			else if(retn)
				cycles.insert(hash);

			return retn;
		}

		bool isReachableRecurse(BasicBlock *from, BasicBlock *to, vector<string> *path)
		{
			if(visited.find(from) != visited.end()) return false;
			if(from == NULL) return false;
			if(from == to)
			{
				path->push_back(from->getName());
				return true;
			}

			visited.insert(from);

			TerminatorInst *term = from->getTerminator();
			int numSucc = term->getNumSuccessors();
			for(int i = 0; i < numSucc; i++)
			{
				BasicBlock *child = term->getSuccessor(i);
				bool reachable = isReachableRecurse(child, to, path);

				if(reachable)
				{
					path->push_back(from->getName());
					return true;
				}
			}

			return false;
		}

		virtual bool runOnFunction(Function &F)
		{
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
						if(succBlock == &(*itr) && isReachable(&(*itr), &(*itr2)))
						{
							errs() << /*F.getName() << "." << */itr->getName()<<" -> ";
							errs() << /*F.getName() << "." << */itr2->getName()<<"\n";
							warshallLoop++;
						}
					}
					
				}
			}

			return false;
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
