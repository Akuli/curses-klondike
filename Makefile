# compile_flags.txt is for clangd
CXXFLAGS += -Wall -Wextra -Wpedantic -Wno-unused-parameter $(shell cat compile_flags.txt) -MMD
LDFLAGS += -lncursesw     # needs cursesw instead of curses for unicodes
IWYU ?= iwyu

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=obj/%.o)
DEPENDS := $(OBJ:.o=.d)
TESTS_SRC := $(wildcard tests/*.cpp)

# valgrind is used in tests
ifdef VALGRIND
VALGRINDOPTS ?= --leak-check=full --show-leak-kinds=all --error-exitcode=1 --errors-for-leak-kinds=all
endif


all: cursesklon test

cursesklon: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -vrf obj cursesklon testrunner

obj/%.o: src/%.cpp
	mkdir -p $(@D) && $(CXX) -c -o $@ $< $(CXXFLAGS)

.PHONY: iwyu
iwyu:
	for file in $(SRC) $(TESTS_SRC); do $(IWYU) $(shell cat compile_flags.txt) $$file; done || true

testrunner: $(TESTS_SRC) $(OBJ)
	$(CXX) -I. $(CXXFLAGS) $(TESTS_SRC) $(filter-out obj/main.o, $(OBJ)) -o testrunner $(LDFLAGS)

.PHONY: test
test: testrunner
	$(VALGRIND) $(VALGRINDOPTS) ./testrunner

-include $(DEPENDS)
