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

template<typename T>
void mergeSet(std::set<T>& s1, const std::set<T> s2) {
    s1.insert(s2.begin(), s2.end());
}
	
///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
///Updated 11/10/2017 by fargo: make all functions
///processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass {
  using FuncCallTableType = std::map<CallInst*, std::set<Function*>, bool(*)(CallInst*, CallInst*)>;

  static char ID; // Pass identification, replacement for typeid
  FuncPtrPass() : ModulePass(ID) {
   // Sort results by row number.
    funcCallTable = FuncCallTableType([](CallInst* p, CallInst *q) {
      if (!p) return true;
      if (!q) return false;
      return p->getDebugLoc().getLine() < q->getDebugLoc().getLine();
    });
  }
  FuncCallTableType funcCallTable;
  std::map<Value*, std::set<Use*>> argTable;
  std::stack<std::pair<CallInst*, Function*>> callStack;

  std::set<Function*> getFunctions(Use& use) {
        std::set<Function*> funcSet;

        if(auto f=dyn_cast<Function>(use)){
          funcSet.insert(f);
        }else if (auto phi = dyn_cast<PHINode>(use)) {
            for (auto& subUse : phi->incoming_values()) {
                mergeSet<Function*>(funcSet, getFunctions(subUse));
            }
        }else if (auto call = dyn_cast<CallInst>(use)) {
            if (auto func = call->getCalledFunction()) {
                callStack.push({call, func});
                mergeSet<Function*>(funcSet,getFunctionsFromRetVal(call,func));
                callStack.pop();
            }
            else {
              for (auto& func : getFunctions(call->getCalledOperandUse())) {
                callStack.push({call, func});
                mergeSet<Function*>(funcSet, getFunctionsFromRetVal(call,func));
                callStack.pop();
                }
            }
        }else if (argTable.count(use)) {
            
            if (callStack.empty()) {
                for (auto subUse : argTable[use]) {
                    mergeSet<Function*>(funcSet, getFunctions(*subUse));
                }
            } else {
                auto call = callStack.top().first;
                auto func = callStack.top().second;
                auto formalArg = func->arg_begin();
                auto actualArg = call->arg_begin();
                while (formalArg != func->arg_end() && actualArg != call->arg_end()) {
                    if (use == formalArg) {
                        callStack.pop();
                        mergeSet<Function*>(funcSet, getFunctions(*actualArg));
                        callStack.push({call, func});
                        break;
                    }
                    ++formalArg;
                    ++actualArg;
                }
        }
      }
    return funcSet;
  }

  std::set<Function*> getFunctionsFromRetVal(CallInst* call,Function* func) {
        std::set<Function*> funcSet;
        for (auto block = func->begin(); block != func->end(); ++block) {
            for (auto inst = block->begin(); inst != block->end(); ++inst) {
                if (auto ret = dyn_cast<ReturnInst>(inst)) {
                    for (auto& subUse : ret->getReturnValue()->uses()) {
                        mergeSet<Function*>(funcSet, getFunctions(subUse));
                    }
                }
            }
        }
        return funcSet;
    }

  void updateArgTable(Function* func, CallInst* call) {
    assert(func->arg_size() == call->arg_size());
    auto formalArg = func->arg_begin();
    auto actualArg = call->arg_begin();
    while (formalArg != func->arg_end() && actualArg != call->arg_end()) {
     if (!argTable.count(formalArg)) {
       argTable[formalArg] = {};
      }
      argTable[formalArg].insert(actualArg);
      ++formalArg;
      ++actualArg;
    }
  }

  bool runOnModule(Module &M) override {
    while (true) {
      auto oldFuncCallTable = funcCallTable;
      for (auto func = M.begin(); func != M.end(); ++func) {
        for (auto block = func->begin(); block != func->end(); ++block) {
          for (auto inst = block->begin(); inst != block->end(); ++inst) {
            if (auto call = dyn_cast<CallInst>(inst)) {                           
              if (auto calledFunc = call->getCalledFunction()) {
                if (call->getCalledFunction()->getName() != "llvm.dbg.value") {
                  if (!funcCallTable.count(call)) {
                    funcCallTable[call] = {};
                  }
                  funcCallTable[call].insert(calledFunc);
                  updateArgTable(calledFunc, call);
                }
              }else {
                Use& use = call->getCalledOperandUse();
                auto funcSet = getFunctions(use);
                if (!funcCallTable.count(call)) {
                  funcCallTable[call] = {};
                }
                for (auto calledFunc : funcSet) {
                  funcCallTable[call].insert(calledFunc);
                  updateArgTable(calledFunc, call);
                }
               }
            }
          }
        }
      }
      if (funcCallTable == oldFuncCallTable) {
        for (auto funcCall : funcCallTable) {
          auto call = funcCall.first; 
          auto funcSet = funcCall.second;
          assert(funcSet.size() != 0);
          errs() << call->getDebugLoc().getLine() << " : ";
          auto it = funcSet.begin();
          errs() << (*it++)->getName();
          while (it != funcSet.end()) {
            errs() << ", " << (*it++)->getName();
          }
          errs() << "\n";
          }
          break;
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

