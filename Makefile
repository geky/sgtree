TARGET = sgtree
LIBTARGET = sgtree.a

CC = g++ --std=c++11
AR = ar
SIZE = size

SRC += $(wildcard *.cpp)
OBJ += $(SRC:.cpp=.o)
DEP += $(SRC:.cpp=.d)
ASM += $(SRC:.cpp=.s)

ifdef DEBUG
CFLAGS += -O0 -g3
else
CFLAGS += -O2
endif
ifdef WORD
CFLAGS += -m$(WORD)
endif
CFLAGS += -include stdio.h
CFLAGS += -Wall -Winline

LFLAGS += -lm


all: $(TARGET)

lib: $(LIBTARGET)

asm: $(ASM)

size: $(OBJ) $(TARGET)
	$(SIZE) $^

-include $(DEP)


$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@

$(LIBTARGET): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.cpp
	$(CC) -c -MMD $(CFLAGS) $< -o $@

%.s: %.cpp
	$(CC) -S $(CFLAGS) $< -o $@


clean:
	rm -f $(TARGET) $(LIBTARGET)
	rm -f $(OBJ)
	rm -f $(DEP)
	rm -f $(ASM)
