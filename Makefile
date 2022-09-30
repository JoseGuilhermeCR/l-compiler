OUTPUT := l-compiler

SRC := src/main.c

OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

C := gcc

C_FLAGS := -Wall -Wextra -Wshadow
C_FLAGS += -std=gnu11

ifdef DEBUG_SYMBOLS
C_FLAGS += -ggdb3
endif

ifdef OPTIMIZATION_LEVEL
C_FLAGS += -O$(OPTIMIZATION_LEVEL)
endif

INCLUDE := -Iinclude
DEFINES :=
LINKER_FLAGS := 

ifdef ASSERT_UNREACHABLE
DEFINES += -DASSERT_UNREACHABLE
endif

%.o: %.c
	$(C) $(C_FLAGS) $(INCLUDE) $(DEFINES) -c $< -o $@

$(OUTPUT): $(OBJ)
	$(C) $^ $(LINKER_FLAGS) -o $@

-include $(DEP)

.PHONY: clean cleandep

cleandep:
	rm -f $(OBJ) $(DEP)

clean:
	rm -f $(OUTPUT) $(OBJ) $(DEP)
