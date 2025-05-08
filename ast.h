// ast.h
#pragma once

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstdint>    
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"
using namespace llvm;
// Base AST node
struct ASTNode {
  virtual ~ASTNode() = default;
  virtual llvm::Value* codegen(llvm::LLVMContext &ctx,
                               llvm::IRBuilder<> &builder,
                               llvm::Module *module) = 0;
  virtual void print(int indent = 0) const = 0;
};

// Numeric literal
class NumberExpr : public ASTNode {
public:
  double Val;
  NumberExpr(double val) : Val(val) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ')
              << "NumberExpr: " << Val << "\n";
  }
};
class IntegerExpr : public ASTNode {
  int64_t Val;
public:
  explicit IntegerExpr(int64_t V) : Val(V) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override {
    // create a 64-bit integer constant
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), Val, true);
  }
  void print(int indent = 0) const override {
    std::cout << std::string(indent,' ')
              << "IntegerExpr: " << Val << "\n";
  }
};
// Variable reference
class VariableExpr : public ASTNode {
public:
  std::string Name;
  VariableExpr(const std::string &name) : Name(name) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ')
              << "VariableExpr: " << Name << "\n";
  }
};

// Variable declaration
class VarDecl : public ASTNode {
public:
  std::string Name;
  ASTNode *Init;
  VarDecl(const std::string &name, ASTNode *init)
    : Name(name), Init(init) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ')
              << "VarDecl: " << Name << "\n";
    Init->print(indent+2);
  }
};

// Echo a string literal
class EchoStr : public ASTNode {
public:
  std::string Str;
  EchoStr(const std::string &s) : Str(s) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ')
              << "EchoStr: \"" << Str << "\"\n";
  }
};

// Echo a variable’s value
class EchoVar : public ASTNode {
public:
  std::string Name;
  EchoVar(const std::string &n) : Name(n) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ')
              << "EchoVar: " << Name << "\n";
  }
};

//Label block label:
class Label : public ASTNode {
  public:
    std::string Name;
    Label(const std::string &n) : Name(n) {}
    llvm::Value* codegen(llvm::LLVMContext &ctx,
                         llvm::IRBuilder<> &builder,
                         llvm::Module *module) override;
    void print(int indent = 0) const override {
      std::cout<<std::string(indent,' ')<<"Label: "<<Name<<"\n";
    }
  };
  
  // unconditional jump saying MOVE TO: label
  class Jump : public ASTNode {
  public:
    std::string Target;
    Jump(const std::string &t) : Target(t) {}
    llvm::Value* codegen(llvm::LLVMContext &ctx,
                         llvm::IRBuilder<> &builder,
                         llvm::Module *module) override;
    void print(int indent = 0) const override {
      std::cout<<std::string(indent,' ')<<"Jump to: "<<Target<<"\n";
    }
  };

  // Binary operator, e.g. lhs + rhs
class BinaryExpr : public ASTNode {
  char Op;
  ASTNode *Left, *Right;
public:
  BinaryExpr(char op, ASTNode *l, ASTNode *r)
    : Op(op), Left(l), Right(r) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent=0) const override {
    std::cout<<std::string(indent,' ')<<"BinaryExpr: "<<Op<<"\n";
    Left->print(indent+2);
    Right->print(indent+2);
  }
};

// Assignment: x = expr
class Assign : public ASTNode {
  std::string LHS;
  ASTNode *RHS;
public:
  Assign(const std::string &lhs, ASTNode *rhs)
    : LHS(lhs), RHS(rhs) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent=0) const override {
    std::cout<<std::string(indent,' ')<<"Assign: "<<LHS<<"\n";
    RHS->print(indent+2);
  }
};

// Comparison expression: lhs < rhs, lhs > rhs
class ComparisonExpr : public ASTNode {
  std::string Op;  // "<" or ">"
  ASTNode *Left, *Right;
public:
  ComparisonExpr(const std::string &op, ASTNode *l, ASTNode *r)
    : Op(op), Left(l), Right(r) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent=0) const override {
    std::cout<<std::string(indent,' ')
             <<"ComparisonExpr: "<<Op<<"\n";
    Left->print(indent+2);
    Right->print(indent+2);
  }
};

// IfStmt: SPIN cond THEN MOVE TO label
class IfStmt : public ASTNode {
  ASTNode *Cond;
  std::string Label;
public:
  IfStmt(ASTNode *c, const std::string &lbl)
    : Cond(c), Label(lbl) {}
  llvm::Value* codegen(llvm::LLVMContext &ctx,
                       llvm::IRBuilder<> &builder,
                       llvm::Module *module) override;
  void print(int indent=0) const override {
    std::cout<<std::string(indent,' ')
             <<"IfStmt: jump to "<<Label<<" if\n";
    Cond->print(indent+2);
  }
};
/// Repeat a block of statements Count times
class Repeat : public ASTNode {
  public:
    double  Count;
    std::vector<ASTNode*> Body;
    Repeat(int  c, std::vector<ASTNode*> *body)
      : Count(c), Body(std::move(*body)) {}
    llvm::Value* codegen(llvm::LLVMContext &ctx,
                         llvm::IRBuilder<> &builder,
                         llvm::Module *module) override;
    void print(int indent=0) const override {
      std::cout<<std::string(indent,' ')
               <<"Repeat "<<Count<<" times\n";
      for (auto *stmt : Body)
        stmt->print(indent+2);
    }
  };


  class ArrayDecl : public ASTNode {
    std::string Name;
    size_t      Count;
  public:
    ArrayDecl(const std::string &n, size_t c)
      : Name(n), Count(c) {}
    llvm::Value* codegen(LLVMContext &ctx,
                         IRBuilder<> &builder,
                         Module *M) override;
    void print(int indent=0) const override {
      std::cout<<std::string(indent,' ')
               <<"ArrayDecl: "<<Name<<"["<<Count<<"]\n";
    }
  };
  
  // arr[idx] so idx can be a expression here so we need ot keep it as a node cus expression is a node
  class IndexExpr : public ASTNode {
    std::string Name;
    ASTNode    *Idx;
  public:
    IndexExpr(const std::string &n, ASTNode *i)
      : Name(n), Idx(i) {}
    llvm::Value* codegen(LLVMContext &ctx,
                         IRBuilder<> &builder,
                         Module *M) override;
    void print(int indent=0) const override {
      std::cout<<std::string(indent,' ')
               <<"IndexExpr: "<<Name<<"[]\n";
      Idx->print(indent+2);
    }
  };
  
  /// name[index] = rhs
  class StoreToIndex : public ASTNode {
    std::string Name;
    ASTNode *Idx, *RHS;
  public:
    StoreToIndex(const std::string &n, ASTNode *i, ASTNode *r)
      : Name(n), Idx(i), RHS(r) {}
    llvm::Value* codegen(LLVMContext &ctx,
                         IRBuilder<> &builder,
                         Module *M) override;
    void print(int indent=0) const override {
      std::cout<<std::string(indent,' ')
               <<"StoreToIndex: "<<Name<<"[] =\n";
      Idx->print(indent+2);
      RHS->print(indent+2);
    }
  };


  class EchoIndexedVar : public ASTNode {
    std::string Name;
    ASTNode    *Idx;
  public:
    EchoIndexedVar(const std::string &n, ASTNode *idx)
      : Name(n), Idx(idx) {}
    llvm::Value* codegen(llvm::LLVMContext &ctx,
                         llvm::IRBuilder<> &builder,
                         llvm::Module *M) override;
    void print(int indent=0) const override {
      std::cout<<std::string(indent,' ')
               <<"EchoIndexedVar: "<<Name<<"[]\n";
      Idx->print(indent+2);
    }
  };
  
