//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hello"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "../../Analysis/Reaching.cpp"
using namespace llvm;

namespace {
  // Hello - The first implementation, without getAnalysisUsage.
  struct Project2 : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Project2() : FunctionPass(ID) {}

	map<Value*, APInt> constantInts;

	virtual bool runOnFunction(Function &F)
	{
		Reaching &reaching = getAnalysis<Reaching>();
		errs() << "Running Constant Transform!\n";

		for(Function::iterator itr = F.begin(); itr != F.end(); itr++)
		{
			//Go through instructions
			for(BasicBlock::iterator blockItr = itr->begin(); blockItr != itr->end(); blockItr++)
			{
				//if(!blockItr->hasName())continue;
				if(StoreInst* SI = dyn_cast<StoreInst>(blockItr))
				//if(LoadInst* SI = dyn_cast<LoadInst>(blockItr))
				{

					errs() << *blockItr << "\n";

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
						errs() << varName << " : " << &*oper2 << "\n";
						constantInts[oper2] = CI->getValue();
						errs() <<"Storing: " << varName << "\n";
					}
					errs() << "\n";
				}
				else //if(UnaryInstruction* UL = dyn_cast<UnaryInstruction>(blockItr))
				{
					errs() << *blockItr << "\n";
					errs() << &*blockItr << " = ";
					int operNum = blockItr->getNumOperands();
					for(int i = 0; i < operNum; i++)
					{
						Value* operand = blockItr->getOperand(i);
						errs() << ", " << operand;

						if(constantInts.find(operand) != constantInts.end())
						{
							errs() << "!!!";
						}
					}
					errs() << "\n";

					//errs() << &*blockItr << "\n";
				}
				/*else if(LoadInst* LI = dyn_cast<LoadInst>(blockItr))
				{
					errs() << *blockItr << "\n";
					errs() << &*blockItr << "\n";
				}*/
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

char Project2::ID = 0;
static RegisterPass<Project2> X("project2Constant", "The constant propagation component of project 2");
