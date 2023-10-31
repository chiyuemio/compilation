//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

#include "Environment.h"

class InterpreterVisitor : 
   public EvaluatedExprVisitor<InterpreterVisitor> {
public:
   explicit InterpreterVisitor(const ASTContext &context, Environment * env)
   : EvaluatedExprVisitor(context), mEnv(env) {}
   virtual ~InterpreterVisitor() {}

   virtual void VisitIntegerLiteral(IntegerLiteral *literal){
      mEnv->literal(literal);
   }

   virtual void VisitBinaryOperator (BinaryOperator * bop) {
	   VisitStmt(bop);
	   mEnv->binop(bop);
   }

   virtual void VisitUnaryOperator(UnaryOperator *uop){
      VisitStmt(uop);
      mEnv->unaryop(uop);
   }

   virtual void VisitDeclRefExpr(DeclRefExpr * expr) {
	   VisitStmt(expr);
	   mEnv->declref(expr);
   }

   virtual void VisitArraySubscriptExpr(ArraySubscriptExpr *arrayexpr){
      VisitStmt(arrayexpr);
      mEnv->array(arrayexpr);
   }
   virtual void VisitParenExpr(ParenExpr *parenexpr){
      VisitStmt(parenexpr);
   }

   virtual void VisitCastExpr(CastExpr * expr) {
	   VisitStmt(expr);
	   mEnv->cast(expr);
   }
   virtual void VisitCallExpr(CallExpr * call) {
	   VisitStmt(call);
      //内建函数不需要后续处理
      if(mEnv->builtinfunc(call)){
         return;
      }
      //创建新栈并进行参数绑定
      mEnv->enterfunc(call);
      //便利函数体
      VisitStmt(call->getDirectCallee()->getBody());
      //弹栈，进行返回值绑定
	   mEnv->exitfunc(call);
   }

   virtual void VisitReturnStmt(ReturnStmt *ret){
      //计算返回值，有点函数无返回值，因此此处不进行返回
      //main函数特殊返回情况：FunctionDecl isMain()
      //没有返回语句
      VisitStmt(ret);
      mEnv->retstmt(ret->getRetValue());
   }

   virtual void VisitDeclStmt(DeclStmt * declstmt) {
      VisitStmt(declstmt);
	   mEnv->decl(declstmt);
   }

   virtual void VisitIfStmt(IfStmt *ifstmt){
      //VisitSTmt()：取出参数的所有子结点进行遍历，但会忽略当前节点本身，因此此处不使用
      Expr *cond=ifstmt->getCond();
      Visit(cond);

      //根据cond判断的结果visit需要执行的子树
      if(mEnv->getExprValue(cond)){
         Visit(ifstmt->getThen());
      }else{
         if(Stmt *elseStmt=ifstmt->getElse()){
            Visit(elseStmt);
         }
      }
   }

   virtual void VisitWhileStmt(WhileStmt *whilestmt){

      Expr *cond=whilestmt->getCond();
      Stmt *body=whilestmt->getBody();

      Visit(cond);

      while(mEnv->getExprValue(cond)){
         Visit(body);
         Visit(cond);
      }
   }

   virtual void VisitForStmt(ForStmt *forstmt){

      Stmt *init=forstmt->getInit();
      Expr *cond=forstmt->getCond();
      Expr *inc=forstmt->getInc();
      Stmt *body=forstmt->getBody();

      if(init){
         Visit(init);
      }
      if(cond){
         Visit(cond);
      }
      

      while(mEnv->getExprValue(cond)){
         Visit(body);
         if(inc){
            Visit(inc);
         }
         if(cond){
            Visit(cond);
         }
         
      }
   }

private:
   Environment * mEnv;
};

class InterpreterConsumer : public ASTConsumer {
public:
   explicit InterpreterConsumer(const ASTContext& context) : mEnv(),
   	   mVisitor(context, &mEnv) {
   }
   virtual ~InterpreterConsumer() {}

   virtual void HandleTranslationUnit(clang::ASTContext &Context) {
	   TranslationUnitDecl * decl = Context.getTranslationUnitDecl();

      //遍历全局变量声明以计算它们的值，保存在临时栈中
      for(TranslationUnitDecl::decl_iterator i=decl->decls_begin(),e=decl->decls_end();i!=e;++i){
         if(VarDecl *vdecl=dyn_cast<VarDecl>(*i)){
            if(vdecl->hasInit()){
               mVisitor.Visit(vdecl->getInit());
            }
         }
      }

	   mEnv.init(decl);

	   FunctionDecl * entry = mEnv.getEntry();
	   mVisitor.VisitStmt(entry->getBody());
  }
private:
   Environment mEnv;
   InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction {
public: 
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new InterpreterConsumer(Compiler.getASTContext()));
  }
};

int main (int argc, char ** argv) {
   if (argc > 1) {
      
      clang::tooling::runToolOnCode(std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), argv[1]);
   }
}
