#include <clang/Driver/Options.h>
#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Rewrite/Core/Rewriter.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/Support/MemoryBuffer.h>
#include <clang/CodeGen/CodeGenAction.h>

#include <cassert>
#include <memory>
#include <string>
#include <fstream>

using namespace std;
using namespace clang;
using namespace llvm;

Rewriter rewriter;
int numFunctions = 0;



template<typename IT>
inline void compile(clang::CompilerInstance *CI,
                    std::string const &FileName,
                    IT FileBegin,
                    IT FileEnd)
{
  auto &CodeGenOpts { CI->getCodeGenOpts() };
  auto &Target { CI->getTarget() };
  auto &Diagnostics { CI->getDiagnostics() };

  // create new compiler instance
  auto CInvNew { std::make_shared<clang::CompilerInvocation>() };

  bool CInvNewCreated {
    clang::CompilerInvocation::CreateFromArgs(
      *CInvNew, CodeGenOpts.CommandLineArgs, Diagnostics) };

  assert(CInvNewCreated);

  clang::CompilerInstance CINew;
  CINew.setInvocation(CInvNew);
  CINew.setTarget(&Target);
  CINew.createDiagnostics();

  // create rewrite buffer
  std::string FileContent { FileBegin, FileEnd };
  auto FileMemoryBuffer { llvm::MemoryBuffer::getMemBufferCopy(FileContent) };

  // create "virtual" input file
  auto &PreprocessorOpts { CINew.getPreprocessorOpts() };
  PreprocessorOpts.addRemappedFile(FileName, FileMemoryBuffer.get());

  // generate code
  clang::EmitObjAction EmitObj;
  CINew.ExecuteAction(EmitObj);

  // clean up rewrite buffer
  FileMemoryBuffer.release();
}

class ExampleVisitor : public RecursiveASTVisitor<ExampleVisitor> {
private:
    ASTContext *astContext; // used for getting additional AST info

public:
    explicit ExampleVisitor(CompilerInstance *CI) 
      : astContext(&(CI->getASTContext())) // initialize private members
    {
        rewriter.setSourceMgr(astContext->getSourceManager(), astContext->getLangOpts());
    }

    virtual bool VisitFunctionDecl(FunctionDecl *func) {
        numFunctions++;
        string funcName = func->getNameInfo().getName().getAsString();
        // string insert = "#include <asm/setjmp.h>\n" \
        //                 "extern int input_end;\n" \
        //                 "extern int jmp_buf_valid;\n" \
        //                 "extern struct jmp_buf_data jmp_buf;\n";
        string insert = "#include <linux/fuzz.h>\n";
        rewriter.InsertText(func->getBeginLoc(), insert, true, true);
        // if (funcName == "do_math") {
        //     rewriter.ReplaceText(func->getLocation(), funcName.length(), "add5");
        //     errs() << "** Rewrote function def: " << funcName << "\n";
        // }
        // for (auto st: func->decls()) {
        //     errs() << st << "\n";
        // }
        return true;
    }

    const Stmt* GetParent(const Stmt *st) {
        const auto& parents = astContext->getParentMapContext().getParents(*st);
        // assert(!parents.empty() && "Can not find parent\n");
        if (parents.empty())
            return nullptr;
        return parents[0].get<Stmt>();
    }

    const Stmt* GetPrevious(const Stmt *st) {
        const CompoundStmt *c = dyn_cast<CompoundStmt>(GetParent(st));
        // assert(c);
        if (!c)
            return nullptr;
        bool next = false;
        for (auto it = c->body_rbegin(); it != c->body_rend(); ++it) {
            if (next)
                return *it;
            if (*it == st)
                next = true;
        }
        // assert(false && "no previous statement found");
        return nullptr;
    }

    // SourceLocation getIfCondLoc(const IfStmt *i) {

    // }

    string FindRV(const ReturnStmt *ret) {
        if (!ret->getRetValue())
            return "";
        if (const ImplicitCastExpr *ic = dyn_cast<ImplicitCastExpr>(ret->getRetValue())) {
            if (const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(ic->getSubExpr())) {
                if (dre->getDecl()->getType()->isIntegerType()) {
                    return dre->getNameInfo().getName().getAsString();
                }
            }
        }
        return "";
    }

    string FindRC(const IfStmt *i) {
        if (const ImplicitCastExpr *ic = dyn_cast<ImplicitCastExpr>(i->getCond())) {
            if (const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(ic->getSubExpr())) {
                if (dre->getDecl()->getType()->isIntegerType()) {
                    return dre->getNameInfo().getName().getAsString();
                }
            }
        }
        else if (const BinaryOperator *ib = dyn_cast<BinaryOperator>(i->getCond())) {
            // to be implemented
            if (ib->getOpcode() == BO_LE)
                if (const ImplicitCastExpr *ic = dyn_cast<ImplicitCastExpr>(ib->getLHS())) {
                    if (const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(ic->getSubExpr())) {
                        if (dre->getDecl()->getType()->isIntegerType()) {
                            return dre->getNameInfo().getName().getAsString();
                        }
                    }
                }
        }
        return "";
    }

    void Modify(const BinaryOperator *bin, const Stmt *i, string end, string rc) {
        static int count = 1000;
        std::string insert ="if (input_end || setjmp(push_jmp_buf())) {\n" \
                            "  printk(KERN_INFO \"early returned\\n\");\n" \
                            "  " + rc + ";\n" \
                            "  " + end + ";\n" \
                            "}\n" \
                            "printk(KERN_INFO \"%s: instr " + to_string(count++) + "\\n\", __func__);\n";
        rewriter.InsertText(bin->getBeginLoc(), insert, true, true);

        rewriter.InsertText(i->getBeginLoc(), "pop_jmp_buf();\n", true, true);
    }

    void VisitIfGoto(const IfStmt *i, const GotoStmt *gt) {

        if (gt->getLabel()->getDeclName().getAsString().find("err") == std::string::npos)
            return;
        const Stmt *prev = GetPrevious(i);
        if (!prev)
            return;
        if (const BinaryOperator *bin = dyn_cast<BinaryOperator>(prev)) {
            errs() << "** found binop\n";
            std::string rc = FindRC(i);

            if (rc == "")
                return;

            std::string label = gt->getLabel()->getDeclName().getAsString();
            string end = "goto " + label;
            rc += " = -5";
            Modify(bin, i, end, rc);
        }
    }

    void VisitIfRet(const IfStmt *i, const ReturnStmt *ret) {
        const Stmt *prev = GetPrevious(i);
        if (!prev)
            return;
        if (const BinaryOperator *bin = dyn_cast<BinaryOperator>(prev)) {
            errs() << "** found binop\n";
            std::string rc = FindRC(i);

            if (rc == "" || rc != FindRV(ret))
                return;

            string end = "return " + rc;
            rc += " = -5";
            Modify(bin, i, end, rc);
        }
    }

    void VisitLabel(const LabelStmt *l) {
        std::string label = l->getDecl()->getNameAsString();
        if (label.find("err") == std::string::npos)
            return;

        const Stmt *prev = GetPrevious(l);
        if (!prev)
            return;
        if (const BinaryOperator *bin = dyn_cast<BinaryOperator>(prev)) {
            errs() << "** found binop\n";
            string rc;
            if (const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(bin->getLHS())) {
                if (dre->getDecl()->getType()->isIntegerType()) {
                    rc = dre->getNameInfo().getName().getAsString();
                }
            }
            if (rc == "")
                return;
            string end = "goto " + label;
            rc += " = -5";
            Modify(bin, l, end, rc);
        }

    }

    virtual bool VisitStmt(Stmt *st) {
        if (ReturnStmt *ret = dyn_cast<ReturnStmt>(st)) {
            errs() << "** found ret\n";
            if (const IfStmt *i = dyn_cast<IfStmt>(GetParent(st))) {
                errs() << "** found if\n";
                VisitIfRet(i, ret);
            }
            else if (const CompoundStmt *c = dyn_cast<CompoundStmt>(GetParent(st))) {
                errs() << "** found compound\n";
                const Stmt *par = GetParent(c);
                if (!par)
                    return true;
                if (const IfStmt *i = dyn_cast<IfStmt>(par)) {
                    errs() << "** found if\n";
                    VisitIfRet(i, ret);
                }
            }
        }
        if (LabelStmt *l = dyn_cast<LabelStmt>(st)) {
            VisitLabel(l);
        }
        if (GotoStmt *gt = dyn_cast<GotoStmt>(st)) {
            errs() << "** found goto\n";
            if (const IfStmt *i = dyn_cast<IfStmt>(GetParent(st))) {
                errs() << "** found if\n";
                VisitIfGoto(i, gt);
            }
            else if (const CompoundStmt *c = dyn_cast<CompoundStmt>(GetParent(st))) {
                errs() << "** found compound\n";
                const Stmt *par = GetParent(c);
                if (!par)
                    return true;
                if (const IfStmt *i = dyn_cast<IfStmt>(par)) {
                    errs() << "** found if\n";
                    VisitIfGoto(i, gt);
                }                
            }
        }
        return true;
    }

/*
    virtual bool VisitReturnStmt(ReturnStmt *ret) {
        rewriter.ReplaceText(ret->getRetValue()->getLocStart(), 6, "val");
        errs() << "** Rewrote ReturnStmt\n";
        return true;
    }

    virtual bool VisitCallExpr(CallExpr *call) {
        rewriter.ReplaceText(call->getLocStart(), 7, "add5");
        errs() << "** Rewrote function call\n";
        return true;
    }
*/
};



class ExampleASTConsumer : public ASTConsumer {
private:
    ExampleVisitor *visitor; // doesn't have to be private

public:
    // override the constructor in order to pass CI
    explicit ExampleASTConsumer(CompilerInstance *CI)
        : visitor(new ExampleVisitor(CI)) // initialize the visitor
    { }

    // override this to call our ExampleVisitor on the entire source file
    virtual void HandleTranslationUnit(ASTContext &Context) {
        /* we can use ASTContext to get the TranslationUnitDecl, which is
             a single Decl that collectively represents the entire source file */
        visitor->TraverseDecl(Context.getTranslationUnitDecl());

        const RewriteBuffer *RewriteBuf =
            rewriter.getRewriteBufferFor(rewriter.getSourceMgr().getMainFileID());
        llvm::outs() << "Rewrited content:\n";
        if (RewriteBuf)
            llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());
        // rewriter.
    }

/*
    // override this to call our ExampleVisitor on each top-level Decl
    virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
        // a DeclGroupRef may have multiple Decls, so we iterate through each one
        for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; i++) {
            Decl *D = *i;    
            visitor->TraverseDecl(D); // recursively visit each AST node in Decl "D"
        }
        return true;
    }
*/
};

class PluginExampleAction : public PluginASTAction {
protected:
    // this gets called by Clang when it invokes our Plugin
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) {
        _CI = &CI;
        _FileName = file.str();
        return std::unique_ptr<ASTConsumer>(new ExampleASTConsumer(&CI));
    }

    // implement this function if you want to parse custom cmd-line args
    bool ParseArgs(const CompilerInstance &CI, const vector<string> &args) {
        return true;
    }

    // Automatically run the plugin after the main AST action
    PluginASTAction::ActionType getActionType() {
        // return AddBeforeMainAction;
        return ReplaceAction;
    }
    
    void EndSourceFileAction() {
        const RewriteBuffer *RewriteBuf =
            rewriter.getRewriteBufferFor(rewriter.getSourceMgr().getMainFileID());

        fstream mod;
        mod.open(_FileName + ".mod", fstream::out | fstream::trunc);
        mod << string(RewriteBuf->begin(), RewriteBuf->end());
        mod.close();
        errs() << "file name: " << _FileName << "\n";
        compile(_CI, _FileName, RewriteBuf->begin(), RewriteBuf->end());
    }
private:
    CompilerInstance *_CI;
    std::string _FileName;
};


static FrontendPluginRegistry::Add<PluginExampleAction> X("-example-plugin", "simple Plugin example");
