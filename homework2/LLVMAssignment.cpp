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

#include <llvm/Support/CommandLine.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#ifdef DEBUG
#define LOG_DEBUG(msg)                                                         \
  do {                                                                         \
    errs() << "[DEBUG] " << msg << "\n";                                       \
  } while (0)
#else
#define LOG_DEBUG(msg)                                                         \
  do {                                                                         \
  } while (0)
#endif

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

  //set去重
  std::map<unsigned int, std::set<std::string>> results;
  std::set<std::string> tempFuncNames;

private:
  void addFuncName(const std::string funcName){
    tempFuncNames.insert(funcName);
    LOG_DEBUG("Add function name: "<<funcName);
  }

  void saveResultAndClearTemp(const unsigned int lineno){
    //自动推断变量类型
    auto result = results.find(lineno);
    if(result==results.end()){
      if(!tempFuncNames.empty()){
        results.insert(std::pair<unsigned int, std::set<std::string>>(lineno, tempFuncNames));
        LOG_DEBUG("Save result for line "<<lineno);
      }
      
    }else{
      //一行出现多个函数调用
      
      auto &funcNames=result->second;
      funcNames.insert(tempFuncNames.begin(),tempFuncNames.end());
    }
    tempFuncNames.clear();
  }

  void printResults() const{
    for(const auto &out: results){
      errs() << out.first <<": ";
      const auto &funcNames = out.second;
      for(auto iter = funcNames.begin(); iter != funcNames.end(); iter++){
        if(iter != funcNames.begin()){
          errs() <<",";
        }
        errs() <<*iter;
      }
      errs()<<"\n";
    }
  }

  //phi结点处理
  void handlePHINode(const PHINode *phiNode){
    for(const Value *income:phiNode->incoming_values()){
      LOG_DEBUG("Incoming value for PHINode: "<<*income);
      if(const Function *func=dyn_cast<Function>(income)){
        handleFunction(func);
      }else if(const PHINode *innerPHINode=dyn_cast<PHINode>(income)){
        handlePHINode(innerPHINode);
      }else if(const Argument * arg=dyn_cast<Argument>(income)){
        handleArgument(arg);
      }else{
        LOG_DEBUG("Unhandled PHINode incoming value: "<<*income);
      }
    }
  }

  void handleFunction(const Function *func){
    std::string funcName=func->getName().data();
    if(funcName!="llvm.dbg.value"){
      addFuncName(func->getName().data());
    }
  }

  //处理Argument->找到形参Argument
  //从所在函数的所有调用中获取实参，再递归处理
  void handleArgument(const Argument *arg){
    //获取形参位置
    const unsigned int argIdx=arg->getArgNo();
    //获取形参所在的函数
    const Function *parentFunc=arg->getParent();

    for(const User *user:parentFunc->users()){
      if(const CallInst *callInst=dyn_cast<CallInst>(user)){
        assert(callInst->getCalledFunction()==parentFunc);
        Value *operand=callInst->getArgOperand(argIdx);
        handleValue(operand);
      }else if(const PHINode *phiNode=dyn_cast<PHINode>(user)){
        for(const User *phiUser:phiNode->users()){
          if(const CallInst *outerCallInst=dyn_cast<CallInst>(phiUser)){
            Value *operand=outerCallInst->getArgOperand(argIdx);
            handleValue(operand);
          }else{
          LOG_DEBUG("UNhandled user of PHINode: "<<*user);
          }
        }
      }else{
        LOG_DEBUG("Unhandled user of parent function of argument: "<<*user);
      }
    } 
  }

  void handleValue(const Value *value){
    if(const PHINode *phiNode=dyn_cast<PHINode>(value)){
      handlePHINode(phiNode);
    }else if(const Argument *arg=dyn_cast<Argument>(value)){
      handleArgument(arg);
    }else if(const CallInst *call=dyn_cast<CallInst>(value)){
      LOG_DEBUG("Handling CallInst in function handleValue: "<<*call);
      handleCallInst(call);
    }else if(const Function *func=dyn_cast<Function>(value)){
      LOG_DEBUG("Handling Function in function handleVale: "<<*func);
      handleFunction(func);
    }else{
      LOG_DEBUG("Unhandled Value: "<<*value);
    }
  }

  //CallInst->函数调用，包含调用的函数名、传递给函数的参数
  void handleCallInst(const CallInst *callInst){
    unsigned int lineno=callInst->getDebugLoc().getLine();

    //直接函数调用、或者被优化成直接调用的间接函数调用
    if(const Function *func=callInst->getCalledFunction()){
      if(lineno!=0){
        LOG_DEBUG("Direct function invocation in line"<<lineno<<": "<<*callInst);
      }
      handleFunction(func);
    }else{
      //非直接调用
      LOG_DEBUG("Indirect function invocation: "<<lineno<<": "<<*callInst);
      const Value *operand=callInst->getCalledOperand();
      LOG_DEBUG("value of indirect function invocation: "<<*operand);
      if(const CallInst *innerCallInst = dyn_cast<CallInst>(operand)) {
        
        if(const Function *calledFunc = innerCallInst->getCalledFunction()){
          for (const BasicBlock &bb : *calledFunc) {
          for (const Instruction &i : bb) {
            if (const ReturnInst *retInst = dyn_cast<ReturnInst>(&i)) {
              const Value *retValue = retInst->getReturnValue();
              LOG_DEBUG("Meet return value: " << *retValue);
              if (const Argument *arg = dyn_cast<Argument>(retValue)) {
                handleArgument(arg);
              } else {
                
                LOG_DEBUG("Unhandled return value: " << *retValue);
              }
            }
          }
        }
        }else if(true){
          const Value *innerCallInstOperand=innerCallInst->getCalledOperand();
          LOG_DEBUG("innerCallINstOperand: "<<*innerCallInstOperand);
          if(const PHINode *phiNode=dyn_cast<PHINode>(innerCallInstOperand)){
            for(const Value *income_func:phiNode->incoming_values()){
              if(const Function *calledFunc=dyn_cast<Function>(income_func)){
                for(const BasicBlock &bb:*calledFunc){
                  for(const Instruction &i:bb){
                    if(const ReturnInst *retInst=dyn_cast<ReturnInst>(&i)){
                      const Value *retValue=retInst->getReturnValue();
                      LOG_DEBUG("Meet return value (inner): "<<*retValue);
                      if(const Argument *arg=dyn_cast<Argument>(retValue)){
                        handleArgument(arg);
                      }else{
                        LOG_DEBUG("Unhandled return value: "<<*retValue);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      } else if (const PHINode *phiNode = dyn_cast<PHINode>(operand)) {
        handlePHINode(phiNode);
      } else if (const Argument *arg = dyn_cast<Argument>(operand)) {
        handleArgument(arg);
      } else {
        LOG_DEBUG("Unhandled operand type in CallInst: " << *operand);
      }      
    }
  }
  

public:
  bool runOnModule(Module &M) override {
    errs() << "Hello: ";
    errs().write_escaped(M.getName()) << '\n';
    M.dump();
    errs()<<"------------------------------\n";

    for(const Function &f:M){
      for(const BasicBlock &bb:f){
        for(const Instruction &i:bb){
          if(const CallInst *callInst=dyn_cast<CallInst>(&i)){
            handleCallInst(callInst);
            saveResultAndClearTemp(callInst->getDebugLoc().getLine());
          }else{

          }
        }
      }
    }
    printResults();

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

