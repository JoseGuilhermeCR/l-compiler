OUTPUT := l-compiler

SRC := src/main.c

OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

C := gcc

C_FLAGS := -Wall -Wextra -Wshadow
C_FLAGS += -std=gnu11

INCLUDE := -Iinclude
DEFINES :=
LINKER_FLAGS := 

ifdef DEBUG
C_FLAGS += -O0 -ggdb3
DEFINES += -DASSERT_UNREACHABLE
else
C_FLAGS += -O2
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
