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
   std::map<Decl*, int64_t> mVars;//变量，Decl无法被Visit遍历
   std::map<Stmt*, int64_t> mExprs;//表达式，Stmt可以被遍历
   std::map<Stmt*, int64_t *> mPtrs;
   /// The current stmt
   Stmt * mPC;
   int64_t returnValue;//保存当前栈的返回值，整数
public:
   StackFrame() : mVars(), mExprs(), mPC() {
   }

   void bindDecl(Decl* decl, int64_t val) {
      mVars[decl] = val;
   }    
	bool hasDecl(Decl *decl){
		return (mVars.find(decl)!=mVars.end());
	}
   int64_t getDeclVal(Decl * decl) {
      //assert的作用是先计算表达式 expression 
	  //如果其值为假（即为0），那么它先向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行
	  assert (mVars.find(decl) != mVars.end());
      return mVars.find(decl)->second;
   }

   void bindStmt(Stmt * stmt, int64_t val) {
	   mExprs[stmt] = val;
   }
   bool hasStmt(Stmt *stmt){
	return (mExprs.find(stmt)!=mExprs.end());
   }
   int64_t getStmtVal(Stmt * stmt) {
	#ifndef DEBUG
	   assert (mExprs.find(stmt) != mExprs.end());
	#else
		if(mExprs.find(stmt) == mExprs.end()){
			llvm::errs() << "DEBUG Can't find statement:\n";
			stmt->dump();
			assert(false);
		}
	#endif
	   return mExprs[stmt];
   }

	void bindPtr(Stmt *stmt, int64_t *val) { 
		mPtrs[stmt] = val; 
	}

  	int64_t *getPtr(Stmt *stmt) {
    	assert(mPtrs.find(stmt) != mPtrs.end());
    	return mPtrs[stmt];
  	}

   void setPC(Stmt * stmt) {
	   mPC = stmt;
   }
   Stmt * getPC() {
	   return mPC;
   }

   	void setReturnValue(int64_t value){
		returnValue=value;
	}
	int64_t getReturnValue(){
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
   
   std::map<Decl*, int64_t> gVars;

public:
   /// Get the declartions to the built-in functions
   Environment() : mStack(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL) {
		//初始化栈，临时存储用于计算的全局变量
		mStack.push_back(StackFrame());
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
		   }else if(VarDecl *vdecl=dyn_cast<VarDecl>(*i)){
				//保存全局变量
				Stmt *initStmt=vdecl->getInit();
				if(mStack.back().hasStmt(initStmt)){
					gVars[vdecl]=mStack.back().getStmtVal(initStmt);
				}else{
					gVars[vdecl]=0;//未初始化的全局变量默认为0
				}
		   }
	   }
	   mStack.pop_back();//清除临时栈
	   mStack.push_back(StackFrame());//入口函数main的栈
   }

   FunctionDecl * getEntry() {
	   return mEntry;
   }

   int64_t getExprValue(Expr *expr){	
		return mStack.back().getStmtVal(expr);
   }

	//数组
	void array(ArraySubscriptExpr *arraysubscript){

		//getBase()获得指向数组声明的指针DeclRefExpr
		//getIdx()获得索引
		Expr *base=arraysubscript->getBase();
		Expr *index=arraysubscript->getIdx();

		//假设数组元素均int64类型
		int64_t *basePtr=(int64_t *)mStack.back().getStmtVal(base);
		int64_t indexVal=mStack.back().getStmtVal(index);

		//将ArraySubscript表达式的值保存到栈
		mStack.back().bindPtr(arraysubscript,basePtr+indexVal);
		mStack.back().bindStmt(arraysubscript,*(basePtr+indexVal));
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

	void ueot(UnaryExprOrTypeTraitExpr *ueotexpr) {
    //需要再完善
    	UnaryExprOrTypeTrait kind = ueotexpr->getKind();
    	int64_t result = 0;
    	switch (kind) {
    	default:
      		llvm::errs() << "Unhandled UEOT.";
      	break;
    	case UETT_SizeOf:
      		result = 8; // int64_t 和 int64_t * 两种类型
      		break;
    	}	
    	mStack.back().bindStmt(ueotexpr, result);
  }

  void paren(ParenExpr *parenexpr) {
    mStack.back().bindStmt(parenexpr,mStack.back().getStmtVal(parenexpr->getSubExpr()));
  }

   /// !TODO Support comparison operation
   //二元运算
   void binop(BinaryOperator *bop){

		typedef BinaryOperatorKind Opcode;		
	   Expr * left = bop->getLHS();
	   Expr * right = bop->getRHS();
	   int64_t result=0;//二元表达式的计算结果
	   //赋值运算：=,*=,/=,+=,-=,
	   //算术和逻辑运算：+,-,*,/,%,<<,>>,&,^,|

		//int64_t leftValue=mStack.back().getStmtVal(left);
		int64_t rightValue=mStack.back().getStmtVal(right);

	   if (bop->isAssignmentOp()) {
			
		   if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(left)) {
			   Decl * decl = declexpr->getFoundDecl();
			   mStack.back().bindDecl(decl, rightValue);
		   }else if(isa<ArraySubscriptExpr>(left)){
				int64_t *ptr=mStack.back().getPtr(left);
				*ptr=rightValue;
		   }else if(UnaryOperator *uop=dyn_cast<UnaryOperator>(left)){
				assert(uop->getOpcode() == UO_Deref);
        		int64_t *ptr = mStack.back().getPtr(left);
        		*ptr = rightValue;

		   }else{
			llvm::errs()<<"Can't find assignment operation:\n";
			bop->dump();
	   		}
		   result=rightValue;
	   }else{
			//比较操作
			Opcode opc=bop->getOpcode();

			int64_t leftValue=mStack.back().getStmtVal(left);

			if (left->getType()->isPointerType() && right->getType()->isIntegerType()) {
        		assert(opc == BO_Add || opc == BO_Sub);
        		rightValue *= sizeof(int64_t);
      		} else if (left->getType()->isIntegerType() && right->getType()->isPointerType()) {
        		assert(opc == BO_Add || opc == BO_Sub);
        		leftValue *= sizeof(int64_t);
      		}
			
			//int rightValue=mStack.back().getStmtVal(right);

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
		int64_t result = 0;

		//算术运算：+,-,~,!
		//++，--(前后)
		//地址：&，*
		
		Opcode opc = uop->getOpcode();
    	int64_t value = mStack.back().getStmtVal(uop->getSubExpr());

    	switch (opc) {
    		default: llvm::errs() << "Unhandled unary operator.";
    		case UO_Plus: result = value; break;
    		case UO_Minus: result = -value; break;
    		case UO_Not: result = ~value; break;
    		case UO_LNot: result = !value; break;
    		case UO_Deref: 
				mStack.back().bindPtr(uop, (int64_t *)value);
				result = *(int64_t *)value;
				break;
		}
		mStack.back().bindStmt(uop,result);
   }

   void decl(DeclStmt * declstmt) {
	   for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
			   it != ie; ++ it) {
		   Decl * decl = *it;
		   if (VarDecl * vardecl = dyn_cast<VarDecl>(decl)) {
				
				QualType type=vardecl->getType();

				if(type->isIntegerType()||type->isPointerType()){
					if(vardecl->hasInit()){
						mStack.back().bindDecl(vardecl,mStack.back().getStmtVal(vardecl->getInit()));
					}else{
						mStack.back().bindDecl(vardecl,0);
					}
				}else if(type->isArrayType()){
					const ConstantArrayType *array = dyn_cast<ConstantArrayType>(type.getTypePtr());
					int64_t size = array->getSize().getSExtValue();
				   	int64_t * arrayStorage = new int64_t[size];
					for (int64_t i = 0; i < size; i++) {
						arrayStorage[i] = 0;
					}
					mStack.back().bindDecl(vardecl, (int64_t)arrayStorage);
				}else{
					llvm::errs() << "Unhandled decl type: \n";
		   		    declstmt->dump();
				    type->dump();
				}			   
		   }
	   }
   }

   //为DeclRefExpr复制一份对应的DeclStmt值，便于直接引用
   void declref(DeclRefExpr * declref) {
	   mStack.back().setPC(declref);
	   QualType type=declref->getType();
	   if (type->isIntegerType() || type->isArrayType()||type->isPointerType()) {

		   Decl* decl = declref->getFoundDecl();
		   int64_t val;

		   if(mStack.back().hasDecl(decl)){
				val=mStack.back().getDeclVal(decl);
		   }else{
				assert(gVars.find(decl)!=gVars.end());
				val=gVars[decl];
		   }
		   mStack.back().bindStmt(declref, val);
	   }else if(!type->isFunctionProtoType()){
			llvm::errs() <<"Unhandled declref type:\n";
			declref->dump();
			type->dump();
	   }
   }

   void cast(CastExpr * castexpr) {
	   mStack.back().setPC(castexpr);
	   QualType type=castexpr->getType();

	   if (castexpr->getType()->isIntegerType()||(type->isPointerType() && !type->isFunctionPointerType())) {
		   Expr * expr = castexpr->getSubExpr();
		   int64_t val = mStack.back().getStmtVal(expr);
		   mStack.back().bindStmt(castexpr, val );
	   }else if(!type->isFunctionPointerType()){
		   llvm::errs() << "Unhandled cast type: \n";
		   castexpr->dump();
		   type->dump();
	   }
   }

	//返回值保存到栈
	void retstmt(Expr *retexpr){
		//mStack.back().setReturnValue(mStack.back().getStmtVal(retexpr));
		int64_t retval=mStack.back().getStmtVal(retexpr);
		//if(isa<ArraySubscriptExpr>(retexpr->IgnoreImpCasts())){
		//	retval=*(int64_t *)retval;
		//}
		mStack.back().setReturnValue(retval);
	}
	//创建新栈进行参数绑定
	void enterfunc(CallExpr *callexpr){
		FunctionDecl *callee=callexpr->getDirectCallee();

		int paramCount=callee->getNumParams();
		assert(paramCount==callexpr->getNumArgs());

		StackFrame newFrame=StackFrame();
		callee=callee->getDefinition();
		for(int i=0;i<paramCount;i++){
			newFrame.bindDecl(callee->getParamDecl(i),mStack.back().getStmtVal(callexpr->getArg(i)));
		}
		mStack.push_back(newFrame);
	}
	//弹栈，返回值绑定
	void exitfunc(CallExpr *callexpr){
		int64_t returnValue=mStack.back().getReturnValue();
		mStack.pop_back();
		mStack.back().bindStmt(callexpr,returnValue);
	}

   ///内建函数
   bool builtinfunc(CallExpr * callexpr) {
	   mStack.back().setPC(callexpr);
	   int64_t val = 0;
	   FunctionDecl * callee = callexpr->getDirectCallee();

	   //callee = callee->getDefinition();//

	   if (callee == mInput) {
		  llvm::errs() << "Please Input an Integer Value : ";
		  scanf("%ld", &val);

		  mStack.back().bindStmt(callexpr, val);
		  return true;
	   } else if (callee == mOutput) {
		   Expr * decl = callexpr->getArg(0);
		   val = mStack.back().getStmtVal(decl);
		   llvm::errs() << val;
		   mStack.back().bindStmt(callexpr,0);
		   return true;
	   }else if(callee==mMalloc){
			int64_t size=mStack.back().getStmtVal(callexpr->getArg(0));
			mStack.back().bindStmt(callexpr,(int64_t)malloc(size));
			return true;
	   }else if(callee==mFree){
			int64_t *ptr=(int64_t *)mStack.back().getStmtVal(callexpr->getArg(0));
			free(ptr);
			return true;
	   }else {
		   /// You could add your code here for Function call Return
		   return false;
	   }
   }
};


