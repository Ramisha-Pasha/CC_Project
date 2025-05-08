#include "ast.h"
#include <vector>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/GlobalVariable.h>
#include <map>
using namespace llvm;
using namespace std;
static std::map<std::string, llvm::AllocaInst*> SymbolTable;
std::map<std::string, BasicBlock*> LabelBlocks;
 
 static std::map<std::string, llvm::GlobalVariable*> ArrayTable;  // NEW: only arrays

// Number literal e.g 6
llvm::Value* NumberExpr::codegen(llvm::LLVMContext &ctx,
    llvm::IRBuilder<> &ChoreoBuilder,
    llvm::Module *module) {
return llvm::ConstantFP::get(ctx, llvm::APFloat(Val));
}

//load the value of any variable by looking up in the symbol table
Value* VariableExpr::codegen(LLVMContext &ctx,
    IRBuilder<> &ChoreoBuilder,
    Module *module) {
auto symbolTable_slot = SymbolTable[Name];
//SymbolTable["x"] = someAllocInstPointer; //we can load/read the value of x by this and also store into it
if (!symbolTable_slot)
return nullptr;
Type *elemTy = Type::getDoubleTy(ctx);

// 2) Load the double
return ChoreoBuilder.CreateLoad(elemTy, symbolTable_slot, Name + "_ld");
}


// VarDecl: allocate in entry block and store init value in the symbol table for further lookup

Value* VarDecl::codegen(LLVMContext &ctx,
IRBuilder<> &ChoreoBuilder,
Module *module) {
// Create the Alloca in the entry block of the current function
Function *func = ChoreoBuilder.GetInsertBlock()->getParent();
IRBuilder<> tmpBuilder(&func->getEntryBlock(),
func->getEntryBlock().begin());
AllocaInst *symbolTable_slot = tmpBuilder.CreateAlloca(
Type::getDoubleTy(ctx), nullptr, Name);
SymbolTable[Name] = symbolTable_slot;
Value *openingMove = Init->codegen(ctx, ChoreoBuilder, module);
return ChoreoBuilder.CreateStore(openingMove, symbolTable_slot);
}

//echostr calls puts on string
Value* EchoStr::codegen(LLVMContext &ctx,
    IRBuilder<> &ChoreoBuilder,
    Module *module) {
// Prepare format string "%s\n"
static const char *fmt = "%s\n";
// Build the i8* type for the format and the string
Type *i8Ty    = Type::getInt8Ty(ctx);
Type *i8PtrTy = PointerType::get(i8Ty, 0);

// Build a printf signature: i32 printf(i8*, ...)
std::vector<Type*> printfArgs{ i8PtrTy };
FunctionType *printfTy = FunctionType::get(
Type::getInt32Ty(ctx), // return i32
printfArgs,            // takes i8*
true
);
FunctionCallee printfFunc =
module->getOrInsertFunction("printf", printfTy);

// Create the constant strings
Value *fmtPtr = ChoreoBuilder.CreateGlobalStringPtr(fmt);
Value *strPtr = ChoreoBuilder.CreateGlobalStringPtr(Str);

// Call printf(fmtPtr, strPtr)
return ChoreoBuilder.CreateCall(printfFunc, { fmtPtr, strPtr });
}

//echovar calls print(var)
Value* EchoVar::codegen(LLVMContext &ctx,
IRBuilder<> &ChoreoBuilder,
Module *module) {
// 1) Look up the alloca for the variable
auto *symbolTable_slot = SymbolTable[Name];
if (!symbolTable_slot) return nullptr;

// 2) Load the variable’s value
Value *loaded = ChoreoBuilder.CreateLoad(
  symbolTable_slot->getAllocatedType(),
  symbolTable_slot,
  Name.c_str()
);

// 3) Prepare printf("%f\n", val);
//    (i8* format string, double)
Value *fmtPtr = ChoreoBuilder.CreateGlobalStringPtr("%f\n");

// Build the printf signature: i32 printf(i8*, ...)
Type *i8Ty    = Type::getInt8Ty(ctx);
Type *i8PtrTy = PointerType::getUnqual(i8Ty);
FunctionType *printfTy = FunctionType::get(
  ChoreoBuilder.getInt32Ty(),         // returns i32
  { i8PtrTy },                  // takes one i8* argument (then varargs)
  /*isVarArg=*/true
);
FunctionCallee printfFunc =
  module->getOrInsertFunction("printf", printfTy);

// 4) Call printf and return the call instruction
return ChoreoBuilder.CreateCall(printfFunc, { fmtPtr, loaded });
}

// --------------------------------------------Label: lookup the block of the label, create a branch to it and switch builder insertion point to it
Value* Label::codegen(LLVMContext &ctx,
    IRBuilder<> &ChoreoBuilder,
    Module *module) {
// Look up the block for this label
auto lookUp_label_block = LabelBlocks.find(Name);
if (lookUp_label_block == LabelBlocks.end())
return nullptr;
BasicBlock *BB = lookUp_label_block->second;

// If the current block has no ret or br instruction, branch to the label
if (!ChoreoBuilder.GetInsertBlock()->getTerminator())
ChoreoBuilder.CreateBr(BB);
ChoreoBuilder.SetInsertPoint(BB);
return nullptr;
}

//---------------------------------------------------MOVE TO label( creates an unconditional branch insruction to the target block )
Value* Jump::codegen(LLVMContext &ctx,
    IRBuilder<> &ChoreoBuilder,
    Module *module) {
auto lookUp_label_block = LabelBlocks.find(Target);
if (lookUp_label_block == LabelBlocks.end()) return nullptr;
ChoreoBuilder.CreateBr(lookUp_label_block->second);
// create a dummy basic block so subsequent code(the one written after move to)has somewhere to go cus else it will be lost
//  and unreachable from our ast:)
Function *F = ChoreoBuilder.GetInsertBlock()->getParent();
auto *contBB = BasicBlock::Create(ctx, "cont", F);
ChoreoBuilder.SetInsertPoint(contBB);
return nullptr;
}


// --------------------------------------BinaryExpr= recursively calls the codegen for the left and right subtree and finally apply the operator 
//                                                   on their returned value. e.g left cud be a numberExp and right cud be a numberExp.
Value* BinaryExpr::codegen(LLVMContext &ctx,
    IRBuilder<> &ChoreoBuilder,
    Module *module) {
Value *Lval = Left->codegen(ctx, ChoreoBuilder, module);
Value *Rval = Right->codegen(ctx, ChoreoBuilder, module);
if (!Lval || !Rval) return nullptr;
switch (Op) {
case '+': return ChoreoBuilder.CreateFAdd(Lval, Rval, "addtmp");
case '-': return ChoreoBuilder.CreateFSub(Lval, Rval, "subtmp");
case '*': return ChoreoBuilder.CreateFMul(Lval, Rval, "multmp");
case '/': return ChoreoBuilder.CreateFDiv(Lval, Rval, "divtmp");
default:  return nullptr;
}
}

// --------------------------------------------Assignment= looks up the current value of the varriable(lhs) and recursively calls the codegen on rhs
//                                             to get the new val e.g x=x+5 ---> for this lhs =x, rhs=x+5->codegen(binaryExp)->right=5->codegen(NumberExp)
//                                             left=x->codegen(VariableExp)->finally after the lookup x+5 happens in binaryExp and its returned to the V
//                                             then the current val of x is updated using creatStore instruction( kind of updating the current val of x)
Value* Assign::codegen(LLVMContext &ctx,
IRBuilder<> &ChoreoBuilder,
Module *module) {
auto *symbolTable_slot = SymbolTable[LHS];
if (!symbolTable_slot) return nullptr;
Value *V = RHS->codegen(ctx, ChoreoBuilder, module);
return ChoreoBuilder.CreateStore(V, symbolTable_slot);
}

//--------------------------------------------ComparisonExpr= 
Value* ComparisonExpr::codegen(LLVMContext &ctx,
        IRBuilder<> &ChoreoBuilder,
        Module *module) {
Value *Lval = Left->codegen(ctx, ChoreoBuilder, module);
Value *Rval = Right->codegen(ctx, ChoreoBuilder, module);
if (!Lval||!Rval) return nullptr;
if (Op == "<")
return ChoreoBuilder.CreateFCmpULT(Lval, Rval, "cmptmp");
else // ">"
return ChoreoBuilder.CreateFCmpUGT(Lval, Rval, "cmptmp");
}

// IfStmt (SPIN cond THEN MOVE TO label)
Value* IfStmt::codegen(LLVMContext &ctx,
IRBuilder<> &ChoreoBuilder,
Module *module) {
    errs() << "[codegen] IfStmt: jumping to " << Label << "\n";
// find blocks
auto lookUp_label_block = LabelBlocks.find(Label);
if (lookUp_label_block==LabelBlocks.end()) return nullptr;
BasicBlock *thenBB = lookUp_label_block->second;

// make continuation block
Function *F = ChoreoBuilder.GetInsertBlock()->getParent();
BasicBlock *contBB = BasicBlock::Create(ctx,"ifcont",F);

// cond → i1
Value *condVal = Cond->codegen(ctx,ChoreoBuilder,module);
// branch
ChoreoBuilder.CreateCondBr(condVal, thenBB, contBB);
// continue in contBB
ChoreoBuilder.SetInsertPoint(contBB);
return nullptr;
}

//------------------------------------loop= REPEAT 4 TIMES:  ENDREPEAT
Value* Repeat::codegen(LLVMContext &ctx,
    IRBuilder<> &ChoreoBuilder,
    Module *module) {
Function *F = ChoreoBuilder.GetInsertBlock()->getParent();

// induction variable
AllocaInst *iv = ChoreoBuilder.CreateAlloca(
Type::getDoubleTy(ctx), nullptr, "rep.iv");
ChoreoBuilder.CreateStore(ConstantFP::get(ctx, APFloat(0.0)), iv);

// create blocks
BasicBlock *condBB  = BasicBlock::Create(ctx, "rep.cond", F);
BasicBlock *bodyBB  = BasicBlock::Create(ctx, "rep.body", F);
BasicBlock *afterBB = BasicBlock::Create(ctx, "rep.after", F);

// jump to condition
ChoreoBuilder.CreateBr(condBB);

// condition block
ChoreoBuilder.SetInsertPoint(condBB);
Type *elemTy = iv->getAllocatedType();           // the element type (double)
Value *cur   = ChoreoBuilder.CreateLoad(elemTy, iv, "cur");
Value *limit = ConstantFP::get(ctx, APFloat(Count));
Value *cond  = ChoreoBuilder.CreateFCmpULT(cur, limit, "repcond");
ChoreoBuilder.CreateCondBr(cond, bodyBB, afterBB);

// body block
ChoreoBuilder.SetInsertPoint(bodyBB);
for (ASTNode* stmt : Body)
stmt->codegen(ctx, ChoreoBuilder, module);

// increment
cur = ChoreoBuilder.CreateLoad(elemTy, iv, "cur");    // reload
Value *next = ChoreoBuilder.CreateFAdd(
cur, ConstantFP::get(ctx, APFloat(1.0)), "next");
ChoreoBuilder.CreateStore(next, iv);
ChoreoBuilder.CreateBr(condBB);

// after loop
ChoreoBuilder.SetInsertPoint(afterBB);
return nullptr;
}

//---------------------------------ARRAY NODESSS
Value* ArrayDecl::codegen(LLVMContext &ctx,
    IRBuilder<> &ChoreoBuilder,
    Module *M) {
        ArrayType *ensemble = ArrayType::get(Type::getDoubleTy(ctx), Count); //creates an array type variable double values of size count
        Constant *init   = ConstantAggregateZero::get(ensemble);   //initialized that array to 0
        auto *g = new GlobalVariable(                   //creates a new global variable in the module of arrayType which is mutable 
                                                      //external linkage mean any module outside this can see this global variable and woudl be refering to the same memory
            *M, ensemble, false,
            GlobalValue::ExternalLinkage,
            init, Name);              
      
        
        ArrayTable[Name] = g;
        return g;
}


//--------------------IndexExpr 
Value* IndexExpr::codegen(LLVMContext &ctx,
    IRBuilder<> &builder,
    Module *M) {


 auto arr = ArrayTable.find(Name);       //find the global array from the ArrayTable
 if (arr == ArrayTable.end())
   return nullptr;
 GlobalVariable *g = arr->second;

 // 2) compute the index (double → int)
 Value *idxFP = Idx->codegen(ctx, builder, M);
 Value *idx   = builder.CreateFPToSI(
                  idxFP, Type::getInt32Ty(ctx), "idx_i");

 // 3) GEP into g
 Value *zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
 Value *gep  = builder.CreateInBoundsGEP(
                  g->getValueType(),  // this is [N x double]
                  g,
                  { zero, idx },
                  Name + "_ptr");

 // 4) load the element
 return builder.CreateLoad(Type::getDoubleTy(ctx), gep, Name + "_ld");
}

// 3) StoreToIndex: GEP + store
Value* StoreToIndex::codegen(LLVMContext &ctx,
       IRBuilder<> &builder,
       Module *M) {
        auto it = ArrayTable.find(Name);
        if (it == ArrayTable.end())
          return nullptr;
        GlobalVariable *g = it->second;
      
        Value *idxFP = Idx->codegen(ctx, builder, M);
        Value *idx   = builder.CreateFPToSI(
                         idxFP, Type::getInt32Ty(ctx), "idx_i");
      
        Value *zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
        Value *gep  = builder.CreateInBoundsGEP(
                         g->getValueType(),
                         g,
                         { zero, idx },
                         Name + "_ptr");
      
        Value *val = RHS->codegen(ctx, builder, M);
        return builder.CreateStore(val, gep);
}


Value* EchoIndexedVar::codegen(LLVMContext &ctx,
    IRBuilder<> &builder,
    Module *M) {
// 1) find the global array
auto it = ArrayTable.find(Name);
if (it == ArrayTable.end()) return nullptr;
GlobalVariable *g = it->second;

// 2) compute the index (expr yields a double → cast to i32)
Value *idxFP = Idx->codegen(ctx, builder, M);
Value *idx   = builder.CreateFPToSI(
idxFP,
Type::getInt32Ty(ctx),
Name+"_idx");

// 3) get element pointer: GEP [N x double]* → double*
Value *zero = ConstantInt::get(Type::getInt32Ty(ctx), 0);
Value *gep  = builder.CreateInBoundsGEP(
g->getValueType(),  // array type
g,
{ zero, idx },
Name+"_ptr");

// 4) load the double value
Value *val = builder.CreateLoad(
Type::getDoubleTy(ctx),
gep,
Name+"_ld");

// 5) call printf("%f\n", val)
Type *i8Ty    = Type::getInt8Ty(ctx);
Type *i8PtrTy = PointerType::getUnqual(i8Ty);
FunctionType *printfTy = FunctionType::get(
  builder.getInt32Ty(),         // returns i32
  { i8PtrTy },                  // takes one i8* argument (then varargs)
  /*isVarArg=*/true
);
auto printfFunc = M->getOrInsertFunction("printf", printfTy);

// make a constant "%f\n"
Value *fmt = builder.CreateGlobalStringPtr("%f\n", "fmt");
return builder.CreateCall(printfFunc, { fmt, val });
}


