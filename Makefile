# compile_flags.txt is for clangd
CXXFLAGS += -Wall -Wextra -Wpedantic -Wno-unused-parameter $(shell cat compile_flags.txt)
LDFLAGS += -lncursesw     # needs cursesw instead of curses for unicodes
IWYU ?= iwyu

SRC := $(filter-out src/main.cpp, $(wildcard src/*.cpp))
OBJ := $(SRC:src/%.cpp=obj/%.o)
HEADERS := $(wildcard src/*.hpp)
TESTS_SRC := $(wildcard tests/*.cpp)

# valgrind is used in tests
ifdef VALGRIND
VALGRINDOPTS ?= --leak-check=full --show-leak-kinds=all --error-exitcode=1 --errors-for-leak-kinds=all
endif


all: cursesklon test

cursesklon: src/main.cpp $(OBJ) $(HEADERS)
	$(CXX) $(CXXFLAGS) $< $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -vrf obj cursesklon testrunner

obj/%.o: src/%.cpp $(HEADERS)
	mkdir -p $(@D) && $(CXX) -c -o $@ $< $(CXXFLAGS)

.PHONY: iwyu
iwyu:
	for file in $(SRC) $(TESTS_SRC) src/main.cpp; do $(IWYU) $(shell cat compile_flags.txt) $$file; done || true

testrunner: $(TESTS_SRC) $(OBJ)
	$(CXX) -I. $(CXXFLAGS) $(TESTS_SRC) $(OBJ) -o testrunner $(LDFLAGS)

.PHONY: test
test: testrunner
	$(VALGRIND) $(VALGRINDOPTS) ./testrunner
