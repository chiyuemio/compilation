//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <stdio.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

class StackFrame {
   /// StackFrame maps Variable Declaration to Value //将变量声明映射到值
   /// Which are either integer or addresses (also represented using an Integer value)
   std::map<Decl*, int> mVars;//变量
   std::map<Stmt*, int> mExprs;//表达式
   /// The current stmt
   Stmt * mPC;
   int returnValue;//保存当前栈的返回值，整数
public:
   StackFrame() : mVars(), mExprs(), mPC() {
   }

   void bindDecl(Decl* decl, int val) {
      mVars[decl] = val;
   }    

	bool hasDecl(Decl *decl){
		return (mVars.find(decl)!=mVars.end());
	}

   int getDeclVal(Decl * decl) {
      //assert的作用是先计算表达式 expression 
	  //如果其值为假（即为0），那么它先向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行
	  assert (mVars.find(decl) != mVars.end());
      return mVars.find(decl)->second;
   }
   void bindStmt(Stmt * stmt, int val) {
	   mExprs[stmt] = val;
   }
   int getStmtVal(Stmt * stmt) {
	   assert (mExprs.find(stmt) != mExprs.end());
	   return mExprs[stmt];
   }
   void setPC(Stmt * stmt) {
	   mPC = stmt;
   }
   Stmt * getPC() {
	   return mPC;
   }

   	void setReturnValue(int value){
		returnValue=value;
	}
	int getReturnValue(){
		return returnValue;
	}
};

/// Heap maps address to a value
/*
class Heap {
public:
   int Malloc(int size) ;
   void Free (int addr) ;
   void Update(int addr, int val) ;
   int get(int addr);
};
*/

class Environment {
   std::vector<StackFrame> mStack;

   FunctionDecl * mFree;/// Declartions to the built-in functions
   FunctionDecl * mMalloc;
   FunctionDecl * mInput;
   FunctionDecl * mOutput;

   FunctionDecl * mEntry;
public:
   /// Get the declartions to the built-in functions
   Environment() : mStack(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL) {
   }
   /// Initialize the Environment
   void init(TranslationUnitDecl * unit) {
	   for (TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
		   if (FunctionDecl * fdecl = dyn_cast<FunctionDecl>(*i) ) {
			   if (fdecl->getName().equals("FREE")) mFree = fdecl;
			   else if (fdecl->getName().equals("MALLOC")) mMalloc = fdecl;
			   else if (fdecl->getName().equals("GET")) mInput = fdecl;
			   else if (fdecl->getName().equals("PRINT")) mOutput = fdecl;
			   else if (fdecl->getName().equals("main")) mEntry = fdecl;
		   }
	   }
	   mStack.push_back(StackFrame());
   }

   FunctionDecl * getEntry() {
	   return mEntry;
   }

   int getExprValue(Expr *expr){	
		return mStack.back().getStmtVal(expr);
   }

	//保存IntegerLiteral和CharacterLIteral到栈
	void literal(Expr *expr){
		if(IntegerLiteral *literal = dyn_cast<IntegerLiteral>(expr)){
			//整数
			mStack.back().bindStmt(expr,literal->getValue().getSExtValue());
		}else if(CharacterLiteral *literal = dyn_cast<CharacterLiteral>(expr)){
			//char
			mStack.back().bindStmt(expr,literal->getValue());
		}
	}

   /// !TODO Support comparison operation
   //二元运算
   void binop(BinaryOperator *bop){

		typedef BinaryOperatorKind Opcode;		
	   Expr * left = bop->getLHS();
	   Expr * right = bop->getRHS();
	   int result=0;//二元表达式的计算结果
	   //赋值运算：=,*=,/=,+=,-=,
	   //算术和逻辑运算：+,-,*,/,%,<<,>>,&,^,|

	   if (bop->isAssignmentOp()) {
			//赋值语句,val赋值给left
		   int val = mStack.back().getStmtVal(right);
		   mStack.back().bindStmt(left, val);
		   if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(left)) {
			   Decl * decl = declexpr->getFoundDecl();
			   mStack.back().bindDecl(decl, val);
		   }
		   result=val;
	   }else{
			//比较操作
			Opcode opc=bop->getOpcode();
			int leftValue=mStack.back().getStmtVal(left);
			int rightValue=mStack.back().getStmtVal(right);

			switch (opc)
			{
			case BO_Add: result=leftValue+rightValue; break;
			case BO_Sub: result=leftValue-rightValue; break;
			case BO_Mul: result=leftValue*rightValue; break;
			case BO_Div: result=leftValue/rightValue; break;
			case BO_EQ: result=leftValue==rightValue; break;
			case BO_LT: result=leftValue<rightValue; break;
			case BO_NE: result=leftValue!=rightValue; break;
			case BO_GT: result=leftValue>rightValue; break;
			case BO_LE: result=leftValue<=rightValue; break;
			case BO_GE: result=leftValue>=rightValue; break;

			default:
				llvm::errs()<<"Unhandled binary operator.";
			}
	   }
	   mStack.back().bindStmt(bop,result);
   }

	//一元运算
   void unaryop(UnaryOperator *uop){
		typedef UnaryOperatorKind Opcode;
		int result = 0;

		//算术运算：+,-,~,!
		//++，--(前后)
		//地址：&，*
		
		if(uop->isArithmeticOp()){
			Opcode opc=uop->getOpcode();
			int value=mStack.back().getStmtVal(uop->getSubExpr());

			switch (opc)
			{
			case UO_Plus: result=+value; break;
			case UO_Minus: result=-value; break;
			case UO_Not: result=~value; break;//按位取反，每一位0变1,1变0
			case UO_LNot: result=!value; break;//逻辑取反，真值变为i假值，假值变为真值
			
			default:
				llvm::errs()<<"Unhandled unary operator.";
			}
		}
		mStack.back().bindStmt(uop,result);
   }

   void decl(DeclStmt * declstmt) {
	   for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
			   it != ie; ++ it) {
		   Decl * decl = *it;
		   if (VarDecl * vardecl = dyn_cast<VarDecl>(decl)) {
				
				if(vardecl->hasInit()){
					//新变量声明时初始化
					mStack.back().bindDecl(vardecl,mStack.back().getStmtVal(vardecl->getInit()));
				}else{
					//新变量声明时未初始化
					mStack.back().bindDecl(vardecl, 0);
				}			   
		   }
	   }
   }

   //为DeclRefExpr复制一份对应的DeclStmt值，便于直接引用
   void declref(DeclRefExpr * declref) {
	   mStack.back().setPC(declref);
	   if (declref->getType()->isIntegerType()) {
		   Decl* decl = declref->getFoundDecl();

		   int val = mStack.back().getDeclVal(decl);
		   mStack.back().bindStmt(declref, val);
	   }
   }

   void cast(CastExpr * castexpr) {
	   mStack.back().setPC(castexpr);
	   if (castexpr->getType()->isIntegerType()) {
		   Expr * expr = castexpr->getSubExpr();
		   int val = mStack.back().getStmtVal(expr);
		   mStack.back().bindStmt(castexpr, val );
	   }
   }

	//返回值保存到栈
	void retstmt(Expr *retexpr){
		mStack.back().setReturnValue(mStack.back().getStmtVal(retexpr));
	}
	//创建新栈进行参数绑定
	void enterfunc(CallExpr *callexpr){
		FunctionDecl *callee=callexpr->getDirectCallee();
		int paramCount=callee->getNumParams();
		assert(paramCount==callexpr->getNumArgs());

		StackFrame newFrame=StackFrame();
		for(int i=0;i<paramCount;i++){
			newFrame.bindDecl(callee->getParamDecl(i),mStack.back().getStmtVal(callexpr->getArg(i)));
		}
		mStack.push_back(newFrame);
	}
	//弹栈，返回值绑定
	void exitfunc(CallExpr *callexpr){
		int returnValue=mStack.back().getReturnValue();
		mStack.pop_back();
		mStack.back().bindStmt(callexpr,returnValue);
	}

   ///内建函数
   bool builtinfunc(CallExpr * callexpr) {
	   mStack.back().setPC(callexpr);
	   int val = 0;
	   FunctionDecl * callee = callexpr->getDirectCallee();
	   if (callee == mInput) {
		  llvm::errs() << "Please Input an Integer Value : ";
		  scanf("%d", &val);

		  mStack.back().bindStmt(callexpr, val);
		  return true;
	   } else if (callee == mOutput) {
		   Expr * decl = callexpr->getArg(0);
		   val = mStack.back().getStmtVal(decl);
		   llvm::errs() << val;
		   return true;
	   } else {
		   /// You could add your code here for Function call Return
		   return false;
	   }
   }
};


