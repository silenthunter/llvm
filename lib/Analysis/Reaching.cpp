#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
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

using namespace llvm;
using std::map;
using std::set;
using std::queue;
using std::string;
using std::pair;

namespace
{
	struct Reaching : public FunctionPass
	{

		struct VarSet
		{
			public:
			set<string> variables;
		};

		struct InSet
		{
			public:
			map<BasicBlock*, set<string> > variables;
		};

		static char ID;
		Reaching() : FunctionPass(ID){}

		map<BasicBlock*, VarSet*> genSet;
		map<BasicBlock*, VarSet*> killSet;
		map<BasicBlock*, VarSet*> outSet;
		map<BasicBlock*, InSet*> inSet;
		queue<BasicBlock*> changedBlocks;

		virtual bool runOnFunction(Function &F)
		{
			changedBlocks.push(&F.getEntryBlock());

			BasicBlock* currBlock = NULL;
			while(!changedBlocks.empty())
			//for(Function::iterator itr = F.begin(); itr != F.end(); itr++)
			{
				//errs() << "Blocks"<< changedBlocks.size() << "\n";
				currBlock = changedBlocks.front();
				changedBlocks.pop();
				//currBlock = &*itr;

				for(BasicBlock::iterator itr2 = currBlock->begin(); itr2 != currBlock->end(); itr2++)
				{
					if(itr2->hasName() )
					{
						errs() << itr2->getOpcodeName() << " : ";
						string varName = itr2->getName();

						errs() << varName;

						/*int numOpr = itr2->getNumOperands();
						for(int i = 0; i < numOpr; i++)
						{
							Value* oper = itr2->getOperand(i);
							errs() << "[" << oper->getName() << "]";
						}*/

						bool firstSeen = false;
						VarSet* varset = genSet[currBlock];
						if(varset == NULL)
						{
							varset = new VarSet();
							firstSeen = true;
						}

						varset->variables.insert(varName);

						if(firstSeen) genSet[currBlock] = varset;

						//Kill Set
						VarSet* killset;
						if(firstSeen) killset = new VarSet();
						else killset = killSet[currBlock];

						killset->variables.insert(varName);

						if(firstSeen) killSet[currBlock] = killset;

						errs() << "\n";
					}
				}//End instruction loop

				//Calculate outSet
				VarSet* genset = genSet[currBlock];
				VarSet* killset = killSet[currBlock];

				VarSet* outset = outSet[currBlock];
				InSet* inset = inSet[currBlock];

				bool firstCompute = false;
				if(outset == NULL)
				{
					outset = new VarSet();
					outSet[currBlock] =  outset;
					firstCompute = true;
				}
				if(inset == NULL)
				{
					inset = new InSet();
					inSet[currBlock] =  inset;
				}
				if(killset == NULL)
				{
					killset = new VarSet();
					killSet[currBlock] =  killset;
				}
				if(genset == NULL)
				{
					genset = new VarSet();
					genSet[currBlock] =  genset;
				}

				set<string> newOut;

				//Add IN
				for(map<BasicBlock*, set<string> >::iterator blockIn = inset->variables.begin();
				blockIn != inset->variables.end(); blockIn++)
					for(set<string>::iterator strItr = blockIn->second.begin(); 
					strItr != blockIn->second.end(); strItr++)
						newOut.insert(*strItr);

				//Remove KILL
				for(set<string>::iterator strItr = killset->variables.begin(); 
				strItr != killset->variables.end(); strItr++)
					newOut.erase(*strItr);

				//Add Gen
				for(set<string>::iterator strItr = genset->variables.begin(); 
				strItr != genset->variables.end(); strItr++)
					newOut.insert(*strItr);

				//Has the output changed?
				bool hasChanged = false;
				for(set<string>::iterator strItr = newOut.begin(); 
				strItr != newOut.end(); strItr++)
					if(outset->variables.find(*strItr) == outset->variables.end())
					{
						hasChanged = true;
					}
				for(set<string>::iterator strItr = outset->variables.begin(); 
				strItr != outset->variables.end(); strItr++)
					if(newOut.find(*strItr) == newOut.end())
					{
						hasChanged = true;
					}

				//Clear and replace old variables
				outset->variables.clear();
				for(set<string>::iterator varItr = newOut.begin(); 
				varItr != newOut.end(); varItr++)
					outset->variables.insert(*varItr);

				TerminatorInst* termInst = currBlock->getTerminator();
				//errs() << currBlock->getName() << ": " << termInst->getNumSuccessors() << "\n";
				for(unsigned int i = 0; i < termInst->getNumSuccessors(); i++)
				{
					BasicBlock* nextBlock = termInst->getSuccessor(i);
					//Set next block's inputs
					InSet* nextIn = inSet[nextBlock];

					if(nextIn == NULL)
					{
						nextIn = new InSet();
						inSet[nextBlock] = nextIn;
					}

					nextIn->variables[currBlock] = outset->variables;

					//Queue next blocks
					if(hasChanged || firstCompute)
					{
						changedBlocks.push(nextBlock);
					}
				}


			}

			errs() << "------------------------------------\n";
			errs() << F.getName() << "\n";

			for(Function::iterator itr = F.begin(); itr != F.end(); itr++)
			{
				errs() << itr->getName() << ": ";
				for(set<string>::iterator blockItr = outSet[&*itr]->variables.begin(); 
				blockItr != outSet[&*itr]->variables.end(); blockItr++)
				{
					errs() << *blockItr << ", ";
				}
				/*for(map<BasicBlock*, set<string> >::iterator blockItr = inSet[&*itr]->variables.begin(); 
				blockItr != inSet[&*itr]->variables.end(); blockItr++)
				{
					errs() << blockItr->first->getName() << ", ";
				}*/
				errs() << "\n";
			}
			errs() << "------------------------------------\n";

			return false;
		}
	};

	char Reaching::ID = 0;
	static RegisterPass<Reaching> X("reaching", "CSE6142 Project 2 Reaching Pass", false, true);
}
