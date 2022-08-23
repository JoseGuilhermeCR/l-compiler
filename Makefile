OUTPUT := l-compiler.elf

SRC := src/main.c \
       src/file_view.c

OBJ := $(patsubst %.c,%.o,$(SRC))
DEP := $(patsubst %.c,%.d,$(SRC))

CC := gcc

CC_FLAGS := -Wall -Wextra -Wshadow
CC_FLAGS += -std=gnu11

ifdef DEBUG_SYMBOLS
CC_FLAGS += -ggdb3
endif

ifdef OPTIMIZATION_LEVEL
CC_FLAGS += -O$(OPTIMIZATION_LEVEL)
endif

INCLUDE := -Iinclude
DEFINES :=
LINKER_FLAGS := 

%.o: %.c
	$(CC) $(CC_FLAGS) $(INCLUDE) -c $< -o $@

$(OUTPUT): $(OBJ)
	$(CC) $^ $(LINKER_FLAGS) -o $@

-include $(DEP)

.PHONY: clean cleandep

cleandep:
	rm -f $(OBJ) $(DEP)

clean:
	rm -f $(OUTPUT) $(OBJ) $(DEP)
