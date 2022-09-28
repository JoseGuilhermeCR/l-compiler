OUTPUT := l-compiler

SRC := src/Main.cc \
       src/Lexer.cc \
       src/Syntatic.cc \
       src/SymbolTable.cc \
       src/Token.cc \
       src/Utils.cc

OBJ := $(patsubst %.cc,%.o,$(SRC))
DEP := $(patsubst %.cc,%.d,$(SRC))

CXX := g++

CXX_FLAGS := -Wall -Wextra -Wshadow
CXX_FLAGS += -std=c++20

ifdef DEBUG_SYMBOLS
CXX_FLAGS += -ggdb3
endif

ifdef OPTIMIZATION_LEVEL
CXX_FLAGS += -O$(OPTIMIZATION_LEVEL)
endif

INCLUDE := -Iinclude
DEFINES :=
LINKER_FLAGS := 

%.o: %.cc
	$(CXX) $(CXX_FLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

$(OUTPUT): $(OBJ)
	$(CXX) $^ $(LINKER_FLAGS) -o $@

-include $(DEP)

.PHONY: clean cleandep

cleandep:
	rm -f $(OBJ) $(DEP)

clean:
	rm -f $(OUTPUT) $(OBJ) $(DEP)
