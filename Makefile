export TARGET = tests/tests

export CXX ?= g++
export SIZE ?= size

export CXXFLAGS += -std=c++11 -pedantic -Wall -g3 -I.

ifdef DEBUG
CXXFLAGS += -O0
else ifdef SMALL
CXXFLAGS += -Os
else
CXXFLAGS += -O3
endif


ifdef TEST_SIZE
CXXEXTRAFLAGS += "-DTEST_SIZE=$(TEST_SIZE)"
endif

ifdef TEST_CASES
CXXEXTRAFLAGS += 									\
	-DTEST_CASES="$(shell echo $(TEST_CASES) 		\
		| sed 's/[^,]*/test_case(&)/g')"
endif

ifdef TEST_CLASSES
CXXEXTRAFLAGS += 									\
	-DTEST_CLASSES="$(shell echo $(TEST_CLASSES) 	\
		| sed 's/[^,]*/test_class(&)/g')"
endif


all: $(TARGET)

test: $(TARGET)
	$(TARGET) $(TEST_SIZE)

callgrind: $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $(CXXEXTRAFLAGS) 			\
		-include valgrind/callgrind.h				\
		-DTEST_SETUP=CALLGRIND_TOGGLE_COLLECT		\
		-DTEST_SETDOWN=CALLGRIND_TOGGLE_COLLECT		\
		$< -o $(TARGET)
	valgrind --tool=callgrind 						\
		--callgrind-out-file=tests/callgrind.out 	\
		--collect-atstart=no 						\
		--cache-sim=yes --branch-sim=yes $(TARGET)
	callgrind_annotate tests/callgrind.out --inclusive=yes --auto=yes

massif: $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $(CXXEXTRAFLAGS) $< -o $(TARGET)
	valgrind --tool=massif --massif-out-file=tests/massif.out $(TARGET)
	ms_print tests/massif.out

.PHONY: # Just always rebuild, too many things could touch the target
$(TARGET): $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $(CXXEXTRAFLAGS) $< -o $@

clean:
	rm -f $(TARGET)

