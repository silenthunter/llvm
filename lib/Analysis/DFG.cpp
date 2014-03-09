#ifdef NOTDEF

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include <utility>
#include <iostream>
#include <set>
#include <queue>
#include <stack>
#include <map>
#include <vector>
#include "Reaching.cpp"

using namespace llvm;
using std::map;
using std::set;
using std::queue;
using std::string;
using std::pair;
using std::stack;
using std::vector;

namespace
{
	struct DFG : public FunctionPass
	{

		struct dfgEdge
		{
			public:
			map<BasicBlock*, set<Value*> > data;
		};

		set<vector<BasicBlock*>* > regions;
		map<BasicBlock*, vector<BasicBlock*>* > regionMap;

		static char ID;
		DFG() : FunctionPass(ID){}

		virtual bool runOnFunction(Function &F)
		{
			Reaching &reaching = getAnalysis<Reaching>();

			set<BasicBlock*> seen;

			stack<BasicBlock*> depth;
			depth.push(&F.front());

			vector<BasicBlock*>* currReg = new vector<BasicBlock*>();
			regions.insert(currReg);

			//Get single-entry single-exit regions
			while(depth.size() > 0)
			{
				BasicBlock* currBlock = depth.top();
				depth.pop();
				seen.insert(currBlock);

				//Add this block to the region
				currReg->push_back(currBlock);
				regionMap[currBlock] = currReg;

				//Get the terminator
				TerminatorInst* term = currBlock->getTerminator();
				int numSucc = term->getNumSuccessors();

				if(numSucc > 1)
				{
					currReg = new vector<BasicBlock*>();
				}


				//Add next block in the CFG
				for(int i = 0; i < numSucc; i++)
				{
					BasicBlock* succ = term->getSuccessor(i);
					if(seen.find(succ) == seen.end())
						depth.push(succ);
					else
					{
						//Break apart an existing region if needed
						vector<BasicBlock*>* tgtVec = regionMap[succ];
						vector<BasicBlock*>* newVec = NULL;
						bool landingFound;
						for(vector<BasicBlock*>::iterator itr = tgtVec->begin(); itr != tgtVec->end(); itr++)
						{
							if(*itr == succ && *itr != tgtVec->front())
								landingFound = true;

							if(landingFound)
							{
								if(newVec == NULL)
								{
									newVec = new vector<BasicBlock*>();
									regions.insert(newVec);
								}

								regionMap[*itr] = newVec;
							}
						}
					}
				}
			}

			//Remove unneeded paths
			queue<BasicBlock*> blocks;
			blocks.push(&*F.begin());
			seen.clear();
			while(blocks.size() > 0)
			{
				BasicBlock* currBlock = blocks.front();
				blocks.pop();
				seen.insert(currBlock);

				//Loop through instructions
				for(BasicBlock::iterator itr = currBlock->begin(); itr != currBlock->end(); itr++)
				{
					Instruction* inst = &*itr;

					//We don't care about finding assignments in this pass
					if(!dyn_cast<StoreInst>(inst))
					{
						int numOperands = inst->getNumOperands();
						for(int i = 0; i < numOperands; i++)
						{
							Value* operand = inst->getOperand(i);

							//We only care about variables
							if(dyn_cast<Constant>(operand)) continue;
							if(dyn_cast<BasicBlock>(operand))continue;

							set<BasicBlock*> prevBlocks = reaching.reaches(operand, currBlock);
							if(!prevBlocks.empty())
							{
								for(set<BasicBlock*>::iterator prevItr = prevBlocks.begin();
									prevItr != prevBlocks.end(); prevItr++)
								{
									errs() << "[" << currBlock->getName() << "]";
									errs() << (*prevItr)->getName() << "/" << operand->getName() << "\n";
								}
							}
						}
					}
				}

				//Add next block in the CFG
				TerminatorInst* term = currBlock->getTerminator();
				int numSucc = term->getNumSuccessors();
				for(int i = 0; i < numSucc; i++)
				{
					BasicBlock* succ = term->getSuccessor(i);
					if(seen.find(succ) == seen.end())
						blocks.push(succ);
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

	char Reaching::ID = 0;
	static RegisterPass<Reaching> X("reaching", "CSE6142 Project 2 Reaching Pass", false, true);

	char DFG::ID = 0;
	static RegisterPass<DFG> Y("DFG", "CSE6142 Project 2 DFG Reaching Pass", false, true);
}
#endif
