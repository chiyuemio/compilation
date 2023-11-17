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
#include<llvm/IR/Use.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include<llvm/IR/Instructions.h>

using namespace llvm;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }
/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang will
 * have optnone attribute which would lead to some transform passes disabled, like mem2reg.
 */
struct EnableFunctionOptPass: public FunctionPass {
    static char ID;
    EnableFunctionOptPass():FunctionPass(ID){}
    bool runOnFunction(Function & F) override{
        if(F.hasFnAttribute(Attribute::OptimizeNone))
        {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID=0;

	
///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
///Updated 11/10/2017 by fargo: make all functions
///processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  FuncPtrPass() : ModulePass(ID) {}
  std::map<Value*, std::vector<Use*>> argTable;

  std::set<std::string> getFunctions(Use& use) {
        std::set<std::string> funcSet;
        if (auto phi = dyn_cast<PHINode>(use)) {
            for (auto& subUse : phi->incoming_values()) {
                auto subFuncSet = getFunctions(subUse);
                for (auto& func : subFuncSet) {
                    funcSet.insert(func);
                }
            }
        }
        else if (auto f = dyn_cast<Function>(use)) {
            funcSet.insert(f->getName().str());
        }
        if (!funcSet.size() && argTable.count(use)) {
            for (auto subUse : argTable[use]) {
                auto subFuncSet = getFunctions(*subUse);
                for (auto& func : subFuncSet) {
                    funcSet.insert(func);
                }
            }
        }
        return funcSet;
    }
  
  void getArgTable(Module& M) {
        for (auto func_it = M.begin(); func_it != M.end(); ++func_it) {
            Function &func = *func_it;
            for (auto func_user : func.users()) {
                if (auto call = dyn_cast<CallInst>(func_user)) {
                    auto formalArg = func.arg_begin();
                    auto actualArg = call->arg_begin();
                    while (formalArg != func.arg_end() && actualArg != call->arg_end()) {
                        auto formalArgsName = formalArg->getName().str();
                        if (argTable.count(formalArg)) {
                            argTable[formalArg].push_back(actualArg); 
                        }
                        else {
                            argTable[formalArg] = {actualArg};
                        }
                        ++formalArg;
                        ++actualArg;
                    }
                }
            }
        }
    }

  bool runOnModule(Module &M) override {
        getArgTable(M);
        for (auto func_it = M.begin(); func_it != M.end(); ++func_it) {
            Function &func = *func_it;
            for (auto block_it = func.begin(); block_it != func.end(); ++block_it) {
                BasicBlock &block = *block_it;
                for (auto inst_it = block.begin(); inst_it != block.end(); ++inst_it) {
                    Instruction &inst = *inst_it;
                    if (auto call = dyn_cast<CallInst>(&inst)) {
                        if (call->getCalledFunction()) {
                            auto name = call->getCalledFunction()->getName();
                            if (name != "llvm.dbg.value") {
                                errs() << call->getDebugLoc().getLine() << " : ";
                                errs() << call->getCalledFunction()->getName() << "\n";
                            }
                        }
                        else {
                            Use& use = call->getCalledOperandUse();
                            auto funcSet = getFunctions(use);
                            errs() << call->getDebugLoc().getLine() << " : ";
                            auto it = funcSet.begin();
                            errs() << *it++;
                            while (it != funcSet.end()) {
                                errs() << ", " << *it++;
                            }
                            errs() << "\n";
                        }
                    }
                }
            }
        }
        return false;
    }

};


char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass", "Print function call instruction");

static cl::opt<std::string>
InputFilename(cl::Positional,
              cl::desc("<filename>.bc"),
              cl::init(""));


int main(int argc, char **argv) {
   LLVMContext &Context = getGlobalContext();
   SMDiagnostic Err;
   // Parse the command line to read the Inputfilename
   cl::ParseCommandLineOptions(argc, argv,
                              "FuncPtrPass \n My first LLVM too which does not do much.\n");


   // Load the input module
   std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
   if (!M) {
      Err.print(argv[0], errs());
      return 1;
   }

   llvm::legacy::PassManager Passes;
   	
   ///Remove functions' optnone attribute in LLVM5.0
   Passes.add(new EnableFunctionOptPass());
   ///Transform it to SSA
   Passes.add(llvm::createPromoteMemoryToRegisterPass());

   /// Your pass to print Function and Call Instructions
   Passes.add(new FuncPtrPass());
   Passes.run(*M.get());
}

