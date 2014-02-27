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
			Function::BasicBlockListType &blocks = F.getBasicBlockList();
			for(Function::iterator itr = blocks.begin(); itr != blocks.end(); itr++)
			{
				for(BasicBlock::iterator itr2 = itr->begin(); itr2 != itr->end(); itr2++)
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
						VarSet* varset = genSet[&*itr];
						if(varset == NULL)
						{
							varset = new VarSet();
							firstSeen = true;
						}

						varset->variables.insert(varName);

						if(firstSeen) genSet.insert(pair<BasicBlock*, VarSet*>(&*itr, varset));

						//Kill Set
						VarSet* killset;
						if(firstSeen) killset = new VarSet();
						else killset = killSet[&*itr];

						killset->variables.insert(varName);

						if(firstSeen) killSet.insert(pair<BasicBlock*, VarSet*>(&*itr, killset));

						errs() << "\n";
					}
				}//End instruction loop

				//Calculate outSet
				VarSet* genset = genSet[&*itr];
				VarSet* killset = killSet[&*itr];

				VarSet* outset = outSet[&*itr];
				InSet* inset = inSet[&*itr];

				if(outset == NULL)
				{
					outset = new VarSet();
					outSet.insert(pair<BasicBlock*, VarSet*>(&*itr, outset));
				}
				if(inset == NULL)
				{
					inset = new InSet();
					inSet.insert(pair<BasicBlock*, InSet*>(&*itr, inset));
				}
				if(killset == NULL)
				{
					killset = new VarSet();
					outSet.insert(pair<BasicBlock*, VarSet*>(&*itr, killset));
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
						hasChanged = true;
				for(set<string>::iterator strItr = outset->variables.begin(); 
				strItr != outset->variables.end(); strItr++)
					if(newOut.find(*strItr) == newOut.end())
						hasChanged = true;

				//Clear and replace old variables
				outset->variables.clear();
				for(set<string>::iterator varItr = newOut.begin(); 
				varItr != newOut.end(); varItr++)
					outset->variables.insert(*varItr);

				//Queue next blocks
				TerminatorInst* termInst = itr->getTerminator();
				for(int i = 0; i < termInst->getNumSuccessors(); i++)
					changedBlocks.push(termInst->getSuccessor(i));


			}

			return false;
		}
	};

	char Reaching::ID = 0;
	static RegisterPass<Reaching> X("reaching", "CSE6142 Project 2 Reaching Pass", false, true);
}
