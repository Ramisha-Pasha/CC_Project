%code requires {
  #include <vector>
  #include "ast.h"
}
%{
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "ast.h"       // ASTNode
#include "llvm/IR/BasicBlock.h"
extern FILE* yyin;
extern int   yylex();
extern char* yytext;
extern int   yylineno;
void yyerror(const char *s);
using namespace llvm;
// collect top‐level statements here
static std::vector<ASTNode*> *programStmts = nullptr;
extern std::map<std::string, BasicBlock*> LabelBlocks;

%}

//------------------------------SEMANTIC VALUES-----------------------------------
%union {

  char*                         identifier;
  double                        double_literal;
  char*                         string_literal;
  ASTNode* node;
  std::vector<ASTNode*>*        stmt_list;       //Statement list 
 
}

//------------------------------DEFINING TOKENS-------------------------------------
%token <identifier>       tok_identifier tok_label
%token <double_literal>   tok_double_literal
%token <string_literal>   tok_string_literal
%token                    tok_SPIN      //SPIN
%token                    tok_THEN      //THEN
%token                    tok_less      // <
%token                    tok_greater   // >
%token                    '+' '-' '*' '/' '='
%token                   tok_ENTER    //ENTER
%token                   tok_EXIT     //EXIT
%token                   tok_ecco     //ECCO
%token                   tok_ecco_d   //ECCO_D
%token                   tok_moveto   //MOVE TO
%token                   tok_REPEAT tok_TIMES tok_ENDREPEAT //loop syntax 'REPEAT X TIMES' 
%token                    tok_colon
%token                   tok_lparen tok_rparen tok_comma
%token                    tok_ENSEMBLE     /* ENSEMBLE keyword */
%token                    tok_lbracket     /* ‘[’ */
%token                    tok_rbracket     /* ‘]’ */

/*─── Non‐terminals ───────────────────────────────────────────────────────────*/
%type  <stmt_list>       stmt_list 

%type  <node>            enter_stmt echo_stmt lbl_stmt jmp_stmt if_stmt assign_stmt expr repeat_stmt ensemble_stmt 
/* Precedence: */
%nonassoc tok_less tok_greater      /* comparisons */
%left '+' '-'
%left '*' '/'
/*─── Start symbol ───────────────────────────────────────────────────────────*/
%start program
%%
program:
    stmt_list tok_EXIT    { programStmts = $1; }
  ;

stmt_list:
    /* empty */                 { $$ = new std::vector<ASTNode*>(); }
  | stmt_list enter_stmt        { $1->push_back($2); $$ = $1; }
  | stmt_list echo_stmt         { $1->push_back($2); $$ = $1; }
  | stmt_list lbl_stmt          { $1->push_back($2); $$ = $1; }
  | stmt_list jmp_stmt          { $1->push_back($2); $$ = $1; }
  | stmt_list if_stmt           { $1->push_back($2); $$ = $1; }
  | stmt_list assign_stmt       { $1->push_back($2); $$ = $1; }
  | stmt_list repeat_stmt       { $1->push_back($2); $$ = $1; }
  | stmt_list ensemble_stmt   { $1->push_back($2); $$ = $1; }
  
  
  
  
  ;

enter_stmt:
    tok_ENTER tok_identifier '=' tok_double_literal 
                           { $$ = new VarDecl($2, new NumberExpr($4)); }
  ;

/*──────────────────────────────────────────────────────────────────────────────*/
/* 2 Echo:                        ECCO "hello"     or    ECCO_D x  */
echo_stmt:
    tok_ecco tok_string_literal 
      {
        /* echo a string literal */
        $$ = new EchoStr($2);
      }
  | tok_ecco_d tok_identifier 
      {
        /* echo a variable’s value */
        $$ = new EchoVar($2);
      }
  | tok_ecco_d tok_identifier tok_lbracket expr tok_rbracket
     {
       /* $2 = array name, $4 = AST for index */
       $$ = new EchoIndexedVar(std::string($2), $4);
     }
  ;

//assignment like 'identifier= exp'
assign_stmt:
    tok_identifier '=' expr
   {
    fprintf(stderr, "  Parsed ASSIGN: %s = (AST@%p)\n", $1, $3);
     //create the tree node with lhs name in $1, rhs subtree in $3
     $$ = new Assign($1, $3);
   }
   | tok_identifier tok_lbracket expr tok_rbracket '=' expr 
      { $$ = new StoreToIndex(std::string($1), $3, $6); }
;
// Label declaration like 'labelName:'
lbl_stmt:
    tok_label
  {
    $$ = new Label($1);
  }
;

// Unconditional jump like 'MOVE TO labelName'
jmp_stmt:
    tok_moveto tok_identifier
  {
    $$ = new Jump($2);
  }
;

//if condiiton (spin exp)
if_stmt:
   tok_SPIN expr tok_THEN tok_moveto tok_identifier
 {
  fprintf(stderr, " Parsed SPIN: cond → (AST@%p), label → %s\n",
          $2, $5);
   $$ = new IfStmt($2, $5);
 }
;




//expression
expr:
    tok_double_literal  { $$ = new NumberExpr($1); }
  | tok_identifier      { $$ = new VariableExpr($1); }
  | expr '+' expr       {$$ = new BinaryExpr('+', $1, $3); }
  | expr '-' expr       {$$ = new BinaryExpr('-', $1, $3); }
  | expr '*' expr       {$$ = new BinaryExpr('*', $1, $3); }
  | expr '/' expr       {$$ = new BinaryExpr('/', $1, $3); }
  | expr tok_less expr  {$$ = new ComparisonExpr("<", $1, $3); }
  | expr tok_greater expr {$$ = new ComparisonExpr(">", $1, $3); }
  | tok_lparen expr tok_rparen         {$$ = $2; }
  /* array access: arr[expr] */
  | tok_identifier tok_lbracket expr tok_rbracket
      { $$ = new IndexExpr(std::string($1), $3); }
  
;





//loop  struct like 'REAPEAT 5 TIMES'
repeat_stmt:
    tok_REPEAT tok_double_literal tok_TIMES
      /* we’ll collect the inner stmts into $4: */
      stmt_list
    tok_ENDREPEAT
  {
    fprintf(stderr,
            " Parsed REPEAT %g TIMES with %zu body stmts\n",
            $2, $4->size());
    $$ = new Repeat($2, $4);
  }
  ;
//array declaration like 'ENSEMBLE arrayName[double literal]
ensemble_stmt:
    tok_ENSEMBLE tok_identifier
    tok_lbracket tok_double_literal tok_rbracket 
  {
    /* $2 = name, $4 = size as double */
    $$ = new ArrayDecl(std::string($2),
                       static_cast<size_t>($4));
  }
;




%%







int yylex(); // forward declare

void yyerror(const char *s) {
    fprintf(stderr, "Syntax error at line %d: %s (token: %d)\n", yylineno, s, yylex());
}


extern const char* last_jump_label;
extern FILE *yyin;
int main(int argc, char** argv) {
  FILE* in = (argc>1) ? std::fopen(argv[1], "r") : stdin;
  if (!in) { perror("fopen"); return 1; }
  yyin = in;
  yylineno = 1;
  fprintf(stderr, " [main] Starting yyparse()\n");
  yyparse();
  fprintf(stderr, " [main] yyparse() returned\n");
  if (!programStmts) {
  fprintf(stderr, " Parse failed—no AST built.\n");
  return 1;
}
  fprintf(stderr, "🛠  [main] Setting up LLVM & codegen\n");
  // 1) Set up LLVM
  LLVMContext Context;
  auto TheModule = std::make_unique<Module>("choreo", Context);
  IRBuilder<> Builder(Context);



  // 4) Now create *one* main() and emit the rest of the statements there
  FunctionType *mainFT =
    FunctionType::get(Builder.getInt32Ty(), false);
  Function *mainF = Function::Create(mainFT,
                           Function::ExternalLinkage,
                           "main",
                           TheModule.get());
  BasicBlock *mainBB = BasicBlock::Create(Context, "entry", mainF);
  Builder.SetInsertPoint(mainBB);

  // If you have labels, create their blocks now:
  for (ASTNode *stmt : *programStmts) {
    if (auto *lbl = dynamic_cast<Label*>(stmt)) {
      LabelBlocks[lbl->Name] =
        BasicBlock::Create(Context, lbl->Name, mainF);
    }
  }
  for (ASTNode *stmt : *programStmts) {
    stmt->codegen(Context, Builder, TheModule.get());
  }


  // Finally, return 0 from main
  if (!Builder.GetInsertBlock()->getTerminator())
    Builder.CreateRet(ConstantInt::get(Builder.getInt32Ty(), 0));

  // 5) Print LLVM IR
  TheModule->print(llvm::outs(), nullptr);

  return 0;
}