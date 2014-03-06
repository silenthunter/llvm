#define DEBUG_TYPE "Project2Const"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "../../Analysis/Reaching.cpp"
#include <vector>
#include <map>
#include <iostream>
using namespace llvm;

using std::vector;
using std::map;
using std::string;

namespace {
  // Hello - The first implementation, without getAnalysisUsage.
  struct Project2 : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Project2() : FunctionPass(ID) {}

	map<Value*, APInt> constantInts;
	vector<Instruction*> toErase;

	virtual bool runOnFunction(Function &F)
	{
		Reaching &reaching = getAnalysis<Reaching>();
		errs() << "Running Constant Transform!\n";

		for(Function::iterator itr = F.begin(); itr != F.end(); itr++)
		{
			//Go through instructions
			for(BasicBlock::iterator blockItr = itr->begin(); blockItr != itr->end(); blockItr++)
			{
				if(StoreInst* SI = dyn_cast<StoreInst>(blockItr))
				{

					//errs() << *blockItr << "\n";

					//Check for constant operands
					int numOperands = blockItr->getNumOperands();
					if(numOperands < 2) errs() << "StoreInst with less than 2 operands\n";
					Value* oper1 = blockItr->getOperand(0);
					Value* oper2 = blockItr->getOperand(1);

					string localName = oper2->getName();
					string varName = oper2->getName();
					varName.append("/");
					varName.append(itr->getName());

					//Check if a constant is being stored
					if(llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(oper1))
					{
						//errs() << varName << " : " << &*oper2 << "\n";
						constantInts[oper2] = CI->getValue();
						//errs() <<"Storing(" << oper2 << "): " << CI->getValue() << "\n";

						toErase.push_back(&*blockItr);
					}
				}
				//Remove temporary variables that load constant
				else if(LoadInst* LI = dyn_cast<LoadInst>(blockItr))
				{
					Value* operand = blockItr->getOperand(0);

					if(constantInts.find(operand) != constantInts.end())
					{
						APInt constVal = constantInts[operand];
						constantInts[&*blockItr] = constVal;

						toErase.push_back(&*blockItr);
					}
				}
				//Replace constants
				else
				{
					//errs() << *blockItr << "\n";
					int operNum = blockItr->getNumOperands();
					for(int i = 0; i < operNum; i++)
					{
						Value* operand = blockItr->getOperand(i);

						bool isAvail = reaching.available(operand, &*itr);
						if(isAvail)
							errs() << "Reaching!!!!!!!!!!\n";

						if(constantInts.find(operand) != constantInts.end() && isAvail)
						{

							APInt constInt = constantInts[operand];

							//errs() << "Setting const(" << operand << "): " << constInt << "\n";
							ConstantInt* constInst = ConstantInt::get(F.getContext(), constInt);

							errs() << "---------------------------------------\n";
							errs() << "Before: " << *blockItr << "\n";
							errs() << "Constant = " << *constInst << "\n";
							blockItr->setOperand(i, constInst);
							errs() << "After: " << *blockItr << "\n";

						}
					}
					//errs() << "\n";

					//errs() << &*blockItr << "\n";
				}
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
static RegisterPass<Reaching> X("project2Reaching", "The constant propagation component of project 2");

char Project2::ID = 0;
static RegisterPass<Project2> Y("project2Constant", "The constant propagation component of project 2");
