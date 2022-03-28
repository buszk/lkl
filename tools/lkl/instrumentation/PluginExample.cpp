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
#include <unordered_set>

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
    bool isInt;

public:
    explicit ExampleVisitor(CompilerInstance *CI) 
      : astContext(&(CI->getASTContext())), // initialize private members
      isInt(false)
    {
        rewriter.setSourceMgr(astContext->getSourceManager(), astContext->getLangOpts());
        rewriter.InsertText(astContext->getSourceManager().getLocForStartOfFile(astContext->getSourceManager().getMainFileID()), 
            "#include <linux/fuzz.h>\n");
    }

    virtual bool VisitFunctionDecl(FunctionDecl *func) {
        isInt = func->getReturnType()->isIntegerType();
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

    const Stmt* GetNext(const Stmt *st) {
        const CompoundStmt *c = dyn_cast<CompoundStmt>(GetParent(st));
        // assert(c);
        if (!c)
            return nullptr;
        bool next = false;
        for (auto it = c->body_begin(); it != c->body_end(); ++it) {
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
        /* case: if (ret) */
        if (const ImplicitCastExpr *ic = dyn_cast<ImplicitCastExpr>(i->getCond())) {
            if (const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(ic->getSubExpr())) {
                if (dre->getDecl()->getType()->isIntegerType()) {
                    return dre->getNameInfo().getName().getAsString();
                }
            }
        }
        /* case: if (ret < 0) */
        else if (const BinaryOperator *ib = dyn_cast<BinaryOperator>(i->getCond())) {
            if (ib->getOpcode() == BO_LT) {
                if (const ImplicitCastExpr *ic = dyn_cast<ImplicitCastExpr>(ib->getLHS())) {
                    if (const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(ic->getSubExpr())) {
                        if (dre->getDecl()->getType()->isIntegerType()) {
                            return dre->getNameInfo().getName().getAsString();
                        }
                    }
                }
            }
        }
        return "";
    }

    string FindFname(const BinaryOperator *bin) {
        if (const CallExpr *call = dyn_cast<CallExpr>(bin->getRHS())) {
            if (call->getDirectCallee()) {
                return call->getDirectCallee()->getNameInfo().getName().getAsString();
            }
            else {
                return "*indrect_call*";
            }
        }
        return "";
    }

    void Modify(const BinaryOperator *bin, string end, string rc, string fname) {
        
        const Stmt *next = GetNext(bin);
        std::string insert ="if (input_end || setjmp(push_jmp_buf())) {\n" \
                            "  printk(KERN_INFO \"%s: returned from " + fname + "\\n\", __func__);\n" \
                            "  " + rc + ";\n" \
                            "  " + end + ";\n" \
                            "}\n" \
                            "else {\n" \
                            "  printk(KERN_INFO \"%s: calling  " + fname + "\\n\", __func__);\n  ";
        rewriter.InsertText(bin->getBeginLoc(), insert, true, true);
        insert ="  printk(KERN_INFO \"%s: finished " + fname + "\\n\", __func__);\n" \
                "  pop_jmp_buf();\n" \
                "}\n";
        rewriter.InsertText(next->getBeginLoc(), insert, true, true);
    }

    void Modify(const BinaryOperator *bin, const Stmt *i, string end, string rc, string fname) {
        std::string insert ="if (input_end || setjmp(push_jmp_buf())) {\n" \
                            "  printk(KERN_INFO \"early returned\\n\");\n" \
                            "  " + rc + ";\n" \
                            "  " + end + ";\n" \
                            "}\n" \
                            "printk(KERN_INFO \"%s: instr " + fname + "\\n\", __func__);\n";
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
            // errs() << "** found binop\n";
            std::string rc = FindRC(i);
            string fname = FindFname(bin);

            if (rc == "" || fname == "")
                return;

            std::string label = gt->getLabel()->getDeclName().getAsString();
            string end = "goto " + label;
            rc += " = -5";
            Modify(bin, end, rc, fname);
        }
        else if (const CallExpr *call = dyn_cast<CallExpr>(prev)) {
            if (!call->getDirectCallee())
                return;
            string fname = call->getDirectCallee()->getNameInfo().getName().getAsString();
            if (fname.find("unlock") == string::npos)
                return;
            const Stmt *second = GetPrevious(prev);
            if (const BinaryOperator *bin = dyn_cast<BinaryOperator>(second)) {
                // errs() << "** found binop\n";
                std::string rc = FindRC(i);
                string bfname = FindFname(bin);
                if (rc == "" || bfname == "") {
                    // errs() << "** return code or fname not found\n";
                    // errs() << "** rc " << rc << "\n";
                    // errs() << "** fname " << bfname << "\n";
                    return;
                }

                std::string label = gt->getLabel()->getDeclName().getAsString();
                string end = "goto " + label;
                rc += " = -5";
                Modify(bin, "", rc, bfname);
            }
        }
    }

    void VisitIfRet(const IfStmt *i, const ReturnStmt *ret) {
        const Stmt *prev = GetPrevious(i);
        if (!prev)
            return;
        if (const BinaryOperator *bin = dyn_cast<BinaryOperator>(prev)) {
            // errs() << "** found binop\n";
            string end;
            std::string rc = FindRC(i);
            string fname = FindFname(bin);

            if (rc == "" || rc != FindRV(ret) || fname == "")
                return;

            if (isInt) {
                end = "return " + rc;
                rc += " = -5";
                Modify(bin, end, rc, fname);
            }
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
            // errs() << "** found binop\n";
            string rc;
            if (const DeclRefExpr *dre = dyn_cast<DeclRefExpr>(bin->getLHS())) {
                if (dre->getDecl()->getType()->isIntegerType()) {
                    rc = dre->getNameInfo().getName().getAsString();
                }
            }
            string fname = FindFname(bin);
            if (rc == "" || fname == "")
                return;
            string end = "goto " + label;
            rc += " = -5";
            // errs() << "** Calling modify\n";
            Modify(bin, end, rc, fname);
        }

    }

    virtual bool VisitStmt(Stmt *st) {
        if (ReturnStmt *ret = dyn_cast<ReturnStmt>(st)) {
            // errs() << "** found ret\n";
            if (const IfStmt *i = dyn_cast<IfStmt>(GetParent(st))) {
                // errs() << "** found if\n";
                VisitIfRet(i, ret);
            }
            else if (const CompoundStmt *c = dyn_cast<CompoundStmt>(GetParent(st))) {
                // errs() << "** found compound\n";
                const Stmt *par = GetParent(c);
                if (!par)
                    return true;
                if (const IfStmt *i = dyn_cast<IfStmt>(par)) {
                    // errs() << "** found if\n";
                    VisitIfRet(i, ret);
                }
            }
        }
        if (LabelStmt *l = dyn_cast<LabelStmt>(st)) {
            VisitLabel(l);
        }
        if (GotoStmt *gt = dyn_cast<GotoStmt>(st)) {
            // errs() << "** found goto\n";
            if (const IfStmt *i = dyn_cast<IfStmt>(GetParent(st))) {
                // errs() << "** found if\n";
                VisitIfGoto(i, gt);
            }
            else if (const CompoundStmt *c = dyn_cast<CompoundStmt>(GetParent(st))) {
                // errs() << "** found compound\n";
                const Stmt *par = GetParent(c);
                if (!par)
                    return true;
                if (const IfStmt *i = dyn_cast<IfStmt>(par)) {
                    // errs() << "** found if\n";
                    VisitIfGoto(i, gt);
                }                
            }
        }
        return true;
    }

    virtual bool VisitCallExpr(CallExpr *call) {
        static unordered_set<string> sleep_funcs = \
        {"msleep", "ssleep", "udelay", "mdelay", "ndelay", "usleep_range", "schedule_timeout_uninterruptible"};
        if (call->getDirectCallee()) {
            string fname = call->getDirectCallee()->getNameInfo().getName().getAsString();
            if (sleep_funcs.count(fname) > 0) {
                rewriter.InsertText(call->getBeginLoc(), ";// ");
            }
        }
        return true;
    }
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

        if (!RewriteBuf)
            return;
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
