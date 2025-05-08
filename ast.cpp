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
 static std::map<std::string, llvm::GlobalVariable*> ArrayTable;  // holds array names

// ----------------------------------------------------------Number literal e.g 6
llvm::Value* NumberExpr::codegen(llvm::LLVMContext &ChoreoContext,
    llvm::IRBuilder<> &ChoreoBuilder,
    llvm::Module *ChoreoModule) {
return llvm::ConstantFP::get(ChoreoContext, llvm::APFloat(Val));
}

//-----------------------------------------------------------load the value of any variable by looking up in the symbol table
Value* VariableExpr::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
auto symbolTable_slot = SymbolTable[Name];
//SymbolTable["x"] = someAllocInstPointer; //we can load/read the value of x by this and also store into it
if (!symbolTable_slot)
return nullptr;
Type *elemTy = Type::getDoubleTy(ChoreoContext);

//Load the double by CreateLoad command
return ChoreoBuilder.CreateLoad(elemTy, symbolTable_slot, Name + "_ld");
}


//-------------------------------------------------------VarDecl: allocate in entry block and store init value in the symbol table for further lookup

Value* VarDecl::codegen(LLVMContext &ChoreoContext,
IRBuilder<> &ChoreoBuilder,
Module *ChoreoModule) {
// Create the Alloca in the entry block of the current function
Function *func = ChoreoBuilder.GetInsertBlock()->getParent();
// Create a mini‐builder positioned at the top of the entry block.
// That ensures all stack allocations happen before any other code.
IRBuilder<> tmpBuilder(&func->getEntryBlock(),
func->getEntryBlock().begin());
//Allocate a 'double' slot on the stack
AllocaInst *symbolTable_slot = tmpBuilder.CreateAlloca(
Type::getDoubleTy(ChoreoContext), nullptr, Name);
// Remember this stack slot in our symbol table.
SymbolTable[Name] = symbolTable_slot;
//Store that initial value into our newly allocated slot.
//Create a store instruction 
Value *openingMove = Init->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
return ChoreoBuilder.CreateStore(openingMove, symbolTable_slot);
}

//----------------------------------------------------------echostr calls a printFunction on the formatted string
Value* EchoStr::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
// Prepare format string "%s\n"
static const char *fmt = "%s\n";
// Build the i8* type for the format and the string
Type *i8Ty    = Type::getInt8Ty(ChoreoContext);
Type *i8PtrTy = PointerType::get(i8Ty, 0);

// Build a printf signature: i32 printf(i8*, ...)
std::vector<Type*> printfArgs{ i8PtrTy };   //printf function takes i8* pointer types as arguements 
FunctionType *printfTy = FunctionType::get(
Type::getInt32Ty(ChoreoContext), // return i32
printfArgs,            // takes i8*
true
);
//fetch the printf function in the module
FunctionCallee printfFunc =
ChoreoModule->getOrInsertFunction("printf", printfTy);

// create the global constant string for formatted an dour original string
Value *formatStrPtr = ChoreoBuilder.CreateGlobalStringPtr(fmt);
Value *strPtr = ChoreoBuilder.CreateGlobalStringPtr(Str);

// Call printf(formatStrPtr, strPtr)
return ChoreoBuilder.CreateCall(printfFunc, { formatStrPtr, strPtr });
}

//----------------------------------------------------------echovar calls print(var)
Value* EchoVar::codegen(LLVMContext &ChoreoContext,
IRBuilder<> &ChoreoBuilder,
Module *ChoreoModule) {
//find the alloca instant in the symbol table
auto *symbolTable_slot = SymbolTable[Name];
if (!symbolTable_slot) return nullptr;

//Load the variable’s value 
Value *loaded = ChoreoBuilder.CreateLoad(
  symbolTable_slot->getAllocatedType(),
  symbolTable_slot,
  Name.c_str()
);

// 3) Prepare printf("%f\n", val);
//    (i8* format string, double)
Value *formatStrPtr = ChoreoBuilder.CreateGlobalStringPtr("%f\n");

// Build the printf signature: i32 printf(i8*, ...)
Type *i8Ty    = Type::getInt8Ty(ChoreoContext);
Type *i8PtrTy = PointerType::getUnqual(i8Ty);
FunctionType *printfTy = FunctionType::get(
  ChoreoBuilder.getInt32Ty(),         // returns i32
  { i8PtrTy },                  // takes one i8* argument 
  /*isVarArg=*/true
);
FunctionCallee printfFunc =
  ChoreoModule->getOrInsertFunction("printf", printfTy);

//Call printf (formatStr,double)
return ChoreoBuilder.CreateCall(printfFunc, { formatStrPtr, loaded });
}

// --------------------------------------------Label: lookup the block of the label, create a branch to it and switch ChoreoBuilder insertion point to it
Value* Label::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
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
Value* Jump::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
auto lookUp_label_block = LabelBlocks.find(Target);
if (lookUp_label_block == LabelBlocks.end()) return nullptr;
ChoreoBuilder.CreateBr(lookUp_label_block->second);
// create a dummy basic block so subsequent code(the one written after move to)has somewhere to go cus else it will be lost
//  and unreachable from our ast:)
Function *F = ChoreoBuilder.GetInsertBlock()->getParent();
auto *contBB = BasicBlock::Create(ChoreoContext, "cont", F);
ChoreoBuilder.SetInsertPoint(contBB);
return nullptr;
}


// --------------------------------------BinaryExpr= recursively calls the codegen for the left and right subtree and finally apply the operator 
//                                                   on their returned value. e.g left cud be a numberExp and right cud be a numberExp.
Value* BinaryExpr::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
Value *Lval = Left->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
Value *Rval = Right->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
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
Value* Assign::codegen(LLVMContext &ChoreoContext,
IRBuilder<> &ChoreoBuilder,
Module *ChoreoModule) {
auto *symbolTable_slot = SymbolTable[LHS];
if (!symbolTable_slot) return nullptr;
Value *V = RHS->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
return ChoreoBuilder.CreateStore(V, symbolTable_slot);
}

//--------------------------------------------ComparisonExpr = calls codegen for the right and left subtree expression and then returns the 
Value* ComparisonExpr::codegen(LLVMContext &ChoreoContext,
        IRBuilder<> &ChoreoBuilder,
        Module *ChoreoModule) {
Value *Lval = Left->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
Value *Rval = Right->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);

if (!Lval||!Rval) return nullptr;
if (Op == "<")
//create floating point comparison instruction fcmp that returns the value i1( 1 bit integer 1 or 0 as a result)
return ChoreoBuilder.CreateFCmpULT(Lval, Rval, "cmptmp");
else // ">"
return ChoreoBuilder.CreateFCmpUGT(Lval, Rval, "cmptmp");
}

//----------------------------------------------------------IfStmt (SPIN cond THEN MOVE TO label)
Value* IfStmt::codegen(LLVMContext &ChoreoContext,
IRBuilder<> &ChoreoBuilder,
Module *ChoreoModule) {
    errs() << "[codegen] IfStmt: jumping to " << Label << "\n";
// find blocks
auto lookUp_label_block = LabelBlocks.find(Label);
if (lookUp_label_block==LabelBlocks.end()) return nullptr;
//find the name of the label block that we need ot jump on
BasicBlock *thenBB = lookUp_label_block->second;

//declare a continuation block that will be executed in case the condition fails
Function *F = ChoreoBuilder.GetInsertBlock()->getParent();
BasicBlock *contBB = BasicBlock::Create(ChoreoContext,"ifcont",F);

// calls codegen on condition(cond->comparisonExpr) condition → i1
Value *condVal = Cond->codegen(ChoreoContext,ChoreoBuilder,ChoreoModule);
//Create a branch based on the condVal returned if true return to the thenBB else contBB
ChoreoBuilder.CreateCondBr(condVal, thenBB, contBB);
// continue in contBB
ChoreoBuilder.SetInsertPoint(contBB);
return nullptr;
}

//------------------------------------loop= REPEAT 4 TIMES:  ENDREPEAT
Value* Repeat::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
Function *F = ChoreoBuilder.GetInsertBlock()->getParent();

//variable loop count that must be decremented everytime a loop block executes
AllocaInst *loopVariable = ChoreoBuilder.CreateAlloca(
Type::getDoubleTy(ChoreoContext), nullptr, "rep.loopVariable");
ChoreoBuilder.CreateStore(ConstantFP::get(ChoreoContext, APFloat(0.0)), loopVariable);

// create blocks
BasicBlock *condBB  = BasicBlock::Create(ChoreoContext, "rep.cond", F);
BasicBlock *bodyBB  = BasicBlock::Create(ChoreoContext, "rep.body", F);
BasicBlock *afterBB = BasicBlock::Create(ChoreoContext, "rep.after", F);

// jump to condition to check the loop variable. 
ChoreoBuilder.CreateBr(condBB);

// condition block
ChoreoBuilder.SetInsertPoint(condBB);
Type *elemTy = loopVariable->getAllocatedType();           // the element type (double)
Value *cur   = ChoreoBuilder.CreateLoad(elemTy, loopVariable, "cur");
//get the limit of the loop from the count varible
Value *limit = ConstantFP::get(ChoreoContext, APFloat(Count));
//if false meaning if current value have exceeded the limit then create the branh to afterBB  else conitnue to the body block by seting inserting point to it 
Value *cond  = ChoreoBuilder.CreateFCmpULT(cur, limit, "repcond");
ChoreoBuilder.CreateCondBr(cond, bodyBB, afterBB);

//bodyBB
ChoreoBuilder.SetInsertPoint(bodyBB);
//here we have kept a vector or ASTNode representing the body meaning multiple statements. it makes it recursive and so nested repeat is supported
for (ASTNode* stmt : Body)
stmt->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);

//increment the current variable from loopVarible and then store that next value in the loopVariable
cur = ChoreoBuilder.CreateLoad(elemTy, loopVariable, "cur");    // reload
Value *next = ChoreoBuilder.CreateFAdd(
cur, ConstantFP::get(ChoreoContext, APFloat(1.0)), "next");
ChoreoBuilder.CreateStore(next, loopVariable);
ChoreoBuilder.CreateBr(condBB);

//set the insertion poin to the after boby block
ChoreoBuilder.SetInsertPoint(afterBB);
return nullptr;
}

//---------------------------------ARRAY NODESSS
Value* ArrayDecl::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
        ArrayType *ensemble = ArrayType::get(Type::getDoubleTy(ChoreoContext), Count); //creates an array type variable double values of size count
        Constant *init   = ConstantAggregateZero::get(ensemble);   //initialized that array to 0
        auto *g = new GlobalVariable(                   //creates a new global variable in the ChoreoModule of arrayType which is mutable 
                                                      //external linkage mean any ChoreoModule outside this can see this global variable and woudl be refering to the same memory
            *ChoreoModule, ensemble, false,
            GlobalValue::ExternalLinkage,
            init, Name);              
      
        
        ArrayTable[Name] = g;
        return g;
}


//--------------------IndexExpr arr[index] here index cud be any expression and it fetches the returns that value
Value* IndexExpr::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {


 auto arr = ArrayTable.find(Name);       //find the global array from the ArrayTable
 if (arr == ArrayTable.end())
   return nullptr;
 GlobalVariable *g = arr->second;

 //compute the index (double → int) i sreturned by evaluating the expression
 Value *idxFP = Idx->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
 // Convert that double to a 32‐bit integer for GEP
 Value *idx   = ChoreoBuilder.CreateFPToSI(
                  idxFP, Type::getInt32Ty(ChoreoContext), "idx_i");

 // 3) GEP into g
 Value *zero = ConstantInt::get(Type::getInt32Ty(ChoreoContext), 0);
 Value *gep  = ChoreoBuilder.CreateInBoundsGEP(
                  g->getValueType(),  // this is [N x double]
                  g,  //pointer to the global name of array
                  { zero, idx },  //// indices: array base, element offset
                  Name + "_ptr");

 // 4) load the element at that index
 return ChoreoBuilder.CreateLoad(Type::getDoubleTy(ChoreoContext), gep, Name + "_ld");
}

//--------------------------------StoreToIndex: GEP + store     store the value at arr[index]
Value* StoreToIndex::codegen(LLVMContext &ChoreoContext,
       IRBuilder<> &ChoreoBuilder,
       Module *ChoreoModule) {
        auto it = ArrayTable.find(Name);
        if (it == ArrayTable.end())
          return nullptr;
        GlobalVariable *g = it->second;
      
        Value *idxFP = Idx->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
        Value *idx   = ChoreoBuilder.CreateFPToSI(
                         idxFP, Type::getInt32Ty(ChoreoContext), "idx_i");
      
        Value *zero = ConstantInt::get(Type::getInt32Ty(ChoreoContext), 0);
        Value *gep  = ChoreoBuilder.CreateInBoundsGEP(
                         g->getValueType(),
                         g,
                         { zero, idx },
                         Name + "_ptr");
      
        Value *val = RHS->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
        return ChoreoBuilder.CreateStore(val, gep);
}

//---------------------------prints the value of arr[index]
Value* EchoIndexedVar::codegen(LLVMContext &ChoreoContext,
    IRBuilder<> &ChoreoBuilder,
    Module *ChoreoModule) {
//
auto it = ArrayTable.find(Name);
if (it == ArrayTable.end()) return nullptr;
GlobalVariable *g = it->second;

//
Value *idxFP = Idx->codegen(ChoreoContext, ChoreoBuilder, ChoreoModule);
Value *idx   = ChoreoBuilder.CreateFPToSI(
idxFP,
Type::getInt32Ty(ChoreoContext),
Name+"_idx");

//
Value *zero = ConstantInt::get(Type::getInt32Ty(ChoreoContext), 0);
Value *gep  = ChoreoBuilder.CreateInBoundsGEP(
g->getValueType(),  // array type
g,
{ zero, idx },
Name+"_ptr");

//
Value *val = ChoreoBuilder.CreateLoad(
Type::getDoubleTy(ChoreoContext),
gep,
Name+"_ld");

//
Type *i8Ty    = Type::getInt8Ty(ChoreoContext);
Type *i8PtrTy = PointerType::getUnqual(i8Ty);
FunctionType *printfTy = FunctionType::get(
  ChoreoBuilder.getInt32Ty(),         // returns i32
  { i8PtrTy },                  // takes one i8* argument (then varargs)
  /*isVarArg=*/true
);
auto printfFunc = ChoreoModule->getOrInsertFunction("printf", printfTy);

// make a constant "%f\n"
Value *fmt = ChoreoBuilder.CreateGlobalStringPtr("%f\n", "fmt");
return ChoreoBuilder.CreateCall(printfFunc, { fmt, val });
}


