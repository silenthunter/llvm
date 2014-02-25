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

using namespace llvm;
using std::map;

namespace
{
	struct Reaching : public FunctionPass
	{
		static char ID;
		Reaching() : FunctionPass(ID){}

		//map<Block*

		virtual bool runOnFunction(Function &F)
		{
			Function::BasicBlockListType &blocks = F.getBasicBlockList();
			for(Function::iterator itr = blocks.begin(); itr != blocks.end(); itr++)
			{
				for(BasicBlock::iterator itr2 = itr->begin(); itr2 != itr->end(); itr2++)
				{
					if(itr2->getNumOperands() > 0)
					{
						errs() << itr2->getOpcodeName() << " : ";
						for(unsigned int i = 0; i < itr2->getNumOperands(); i++)
						{
							Value *opr = itr2->getOperand(i);
							errs() << *opr << "||";
							//if(Instruction::classof(opr)) errs() << opr->getName();
							//errs() << "--" << opr->getName() << "--" << "{" << *opr << "}";
						}

						errs() << "\n";
					}
				}
			}

			return false;
		}
	};

	char Reaching::ID = 0;
	static RegisterPass<Reaching> X("reaching", "CSE6142 Project 2 Reaching Pass", false, true);
}
