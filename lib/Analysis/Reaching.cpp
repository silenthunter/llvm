#ifndef PROJECT2_REACH
#define PROJECT2_REACH

#define PRINT_REACHING 

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
#include <map>

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
			set<Value*> variables;
			map<Value*, BasicBlock*> sources;
		};

		struct InSet
		{
			public:
			map<BasicBlock*, set<Value*> > variables;
			map<BasicBlock*, map<Value*, BasicBlock*> > sources;
		};

		static char ID;
		Reaching() : FunctionPass(ID){}

		map<BasicBlock*, VarSet*> genSet;
		map<BasicBlock*, VarSet*> killSet;
		map<BasicBlock*, VarSet*> outSet;
		map<BasicBlock*, InSet*> inSet;
		map<BasicBlock*, VarSet*> availSet;
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

				//Loop through all instructions
				for(BasicBlock::iterator itr2 = currBlock->begin(); itr2 != currBlock->end(); itr2++)
				{
					//Is this an assignment instruction?
					if(dyn_cast<StoreInst>(itr2) || dyn_cast<LoadInst>(itr2))
					{

						Instruction* val = &*itr2;
						
						//The second operand is actually being assigned
						if(dyn_cast<StoreInst>(val))
						{
							val = dyn_cast<Instruction>(val->getOperand(1));
						}

						//errs() << val->getOpcodeName() << " : ";
						string localName = val->getName();
						string varName = val->getName();
						varName.append("/");
						varName.append(currBlock->getName());

						//errs() << varName << "\n";

						//Add this variable to the genSet
						bool firstSeen = false;
						VarSet* varset = genSet[currBlock];
						if(varset == NULL)
						{
							varset = new VarSet();
							firstSeen = true;
						}

						varset->variables.insert(val);
						varset->sources[val] = currBlock;

						if(firstSeen) genSet[currBlock] = varset;

						//Kill Set
						VarSet* killset;
						if(firstSeen) killset = new VarSet();
						else killset = killSet[currBlock];

						//Find if this kills an inputted variable
						InSet* inset = inSet[currBlock];
						if(inset != NULL)
						for(map<BasicBlock*, set<Value*> >::iterator blockIn = inset->variables.begin();
						blockIn != inset->variables.end(); blockIn++)
							for(set<Value*>::iterator strItr = blockIn->second.begin(); 
							strItr != blockIn->second.end(); strItr++)
							{
								/*int divideIdx = strItr->find('/');
								bool matchesLocal = strcmp(localName.c_str(),
									strItr->substr(0, divideIdx).c_str()) == 0;*/

								if(*strItr == val)
								{
									//errs() << "Killed: " << (*strItr)->getName()
										//<< "(" << *strItr << ")\n";
									killset->variables.insert(*strItr);
								}
							}


						if(firstSeen) killSet[currBlock] = killset;

						//errs() << "\n";
					}
				}//End instruction loop

				//Calculate outSet
				VarSet* genset = genSet[currBlock];
				VarSet* killset = killSet[currBlock];

				VarSet* outset = outSet[currBlock];
				InSet* inset = inSet[currBlock];

				VarSet* availset = availSet[currBlock];

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
				if(availset == NULL)
				{
					availset = new VarSet();
					availSet[currBlock] =  availset;
				}

				set<Value*> newOut;
				map<Value*, BasicBlock*> newOutSrc;

				//Clear
				availset->variables.clear();
				availset->sources.clear();

				//Add IN
				for(map<BasicBlock*, set<Value*> >::iterator blockIn = inset->variables.begin();
				blockIn != inset->variables.end(); blockIn++)
				{
					for(set<Value*>::iterator strItr = blockIn->second.begin(); 
					strItr != blockIn->second.end(); strItr++)
					{
						newOut.insert(*strItr);
						newOutSrc[*strItr] = inset->sources[blockIn->first][*strItr];

						//Add the first input to the avail set
						if(blockIn == inset->variables.begin())
						{
							availset->variables.insert(*strItr);
							availset->sources[*strItr] = newOutSrc[*strItr];
						}
					}

					//Remove all non-matching entries from the avail set
					if(blockIn != inset->variables.begin())
					{
						set<Value*> toRemove;
						for(set<Value*>::iterator availItr = availset->variables.begin();
							availItr != availset->variables.end(); availItr++)
						{
							//This variable is not available in all inputs
							if(blockIn->second.find(*availItr) == blockIn->second.end())
							{
								toRemove.insert(*availItr);
							}
							else
							{
								BasicBlock* src1 = newOutSrc[*availItr];
								BasicBlock* src2 = availset->sources[*availItr];

								if(src1 != src2)
									toRemove.insert(*availItr);
							}
						}

						//Do the removal
						for(set<Value*>::iterator remItr = toRemove.begin();
							remItr != toRemove.end(); remItr++)
							availset->variables.erase(*remItr);
					}
				}

				//Remove KILL
				for(set<Value*>::iterator strItr = killset->variables.begin(); 
				strItr != killset->variables.end(); strItr++)
				{
					newOut.erase(*strItr);
					newOutSrc.erase(*strItr);
				}

				//Add Gen
				for(set<Value*>::iterator strItr = genset->variables.begin(); 
				strItr != genset->variables.end(); strItr++)
				{
					newOut.insert(*strItr);
					newOutSrc[*strItr] = genset->sources[*strItr];
				}

				//Has the output changed?
				bool hasChanged = false;
				for(set<Value*>::iterator strItr = newOut.begin(); 
				strItr != newOut.end(); strItr++)
					if(outset->variables.find(*strItr) == outset->variables.end())
					{
						hasChanged = true;
					}
				for(set<Value*>::iterator strItr = outset->variables.begin(); 
				strItr != outset->variables.end(); strItr++)
					if(newOut.find(*strItr) == newOut.end())
					{
						hasChanged = true;
					}

				//Clear and replace old variables
				outset->variables.clear();
				outset->sources.clear();
				for(set<Value*>::iterator varItr = newOut.begin(); 
				varItr != newOut.end(); varItr++)
				{
					outset->variables.insert(*varItr);
					outset->sources[*varItr] = newOutSrc[*varItr];
				}

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
					nextIn->sources[currBlock] = outset->sources;

					//Queue next blocks
					if(hasChanged || firstCompute)
					{
						changedBlocks.push(nextBlock);
					}
				}


			}

#ifdef PRINT_REACHING
			errs() << "------------------------------------\n";
			errs() << F.getName() << "\n";

			for(Function::iterator itr = F.begin(); itr != F.end(); itr++)
			{
				errs() << itr->getName() << ": ";
				/*for(set<Value*>::iterator blockItr = outSet[&*itr]->variables.begin(); 
				blockItr != outSet[&*itr]->variables.end(); blockItr++)
				{
					errs() << (*blockItr)->getName() << "/" << outSet[&*itr]->sources[*blockItr]->getName()<< ", ";
				}*/
				for(set<Value*>::iterator blockItr = availSet[&*itr]->variables.begin(); 
				blockItr != availSet[&*itr]->variables.end(); blockItr++)
				{
					errs() << (*blockItr)->getName() << ", ";
				}
				/*for(map<BasicBlock*, set<string> >::iterator blockItr = inSet[&*itr]->variables.begin(); 
				blockItr != inSet[&*itr]->variables.end(); blockItr++)
				{
					errs() << blockItr->first->getName() << ", ";
				}*/
				errs() << "\n";
			}
			errs() << "------------------------------------\n";
#endif

			return false;
		}

		set<BasicBlock*> reaches(Value* variable, BasicBlock* dest)
		{
			set<BasicBlock*> retn;

			InSet* inset = inSet[dest];
			if(inset == NULL) return retn;
			//See if this is an input variable
			for(map<BasicBlock*, map<Value*, BasicBlock*> >::iterator itr = inset->sources.begin();
				itr != inset->sources.end(); itr++)
			{
				if(itr->second.find(variable) != itr->second.end())
				{
					retn.insert(itr->second[variable]);
				}
			}
		
			return retn;
		}

		bool available(Value* variable, BasicBlock* dest)
		{
			VarSet* availset = availSet[dest];
			if(availset == NULL) return false;

			if(availset->variables.find(variable) == availset->variables.end()) return false;

			return true;
		}

		set<Value*> getCombinedInput(BasicBlock* block)
		{
			set<Value*> retn;

			InSet* inset = inSet[block];
			for(map<BasicBlock*, set<Value*> >::iterator blockIn = inset->variables.begin();
			blockIn != inset->variables.end(); blockIn++)
			{
				for(set<Value*>::iterator strItr = blockIn->second.begin(); 
				strItr != blockIn->second.end(); strItr++)
				{
					retn.insert(*strItr);
				}
			}

			return retn;
		}
	};

	//char Reaching::ID = 0;
	///static RegisterPass<Reaching> X("reaching", "CSE6142 Project 2 Reaching Pass", false, true);
}
#endif
