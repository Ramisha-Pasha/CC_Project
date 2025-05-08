# Makefile for ChoreoLang compiler

BISON = bison
FLEX = flex
CXX = g++
CXXFLAGS = -std=c++11
LLVM_CXXFLAGS = $(shell llvm-config --cxxflags)
LLVM_LDFLAGS = $(shell llvm-config --ldflags --libs core)
LEXLIB = -lfl

# Source files
YACC_SRC = choreo1.y
LEX_SRC  = choreo1.l
AST_SRC  = ast.cpp
TARGET   = choreo

# Generated files
YACC_TAB_C = choreo1.tab.c
YACC_TAB_H = choreo1.tab.h
LEX_C      = lex.yy.c

.PHONY: all run clean

all: $(TARGET)

$(YACC_TAB_C) $(YACC_TAB_H): $(YACC_SRC)
	$(BISON) -d $<

$(LEX_C): $(LEX_SRC) $(YACC_TAB_H)
	$(FLEX) $<

$(TARGET): $(YACC_TAB_C) $(LEX_C) $(AST_SRC)
	$(CXX) $(CXXFLAGS) $(LEX_C) $(YACC_TAB_C) $(AST_SRC) $(LEXLIB) $(LLVM_CXXFLAGS) $(LLVM_LDFLAGS) -o $(TARGET)

run: all
	@if [ -z "$(input)" ]; then \
		echo "Usage: make run input=<file.choreo>"; \
	else \
		./$(TARGET) $(input) > out.ll; \
		echo "Generated out.ll"; \
		lli out.ll; \
	fi

clean:
	rm -f $(TARGET) $(LEX_C) $(YACC_TAB_C) $(YACC_TAB_H) out.ll
