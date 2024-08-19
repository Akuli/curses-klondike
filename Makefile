# compile_flags.txt is for clangd
CFLAGS += -g -Wall -Wextra -Wpedantic -Wno-sign-compare $(shell cat compile_flags.txt) -MMD
LDFLAGS += -lncursesw     # needs cursesw instead of curses for unicodes
IWYU ?= iwyu

#SRC := $(wildcard src/*.c)
SRC := src/args.c src/main.c
OBJ := $(SRC:src/%.c=obj/%.o)
DEPENDS := $(OBJ:.o=.d)
#TESTS_SRC := $(wildcard tests/*.c)
TESTS_SRC := tests/run.c tests/test_args.c

# valgrind is used in tests
ifdef VALGRIND
VALGRINDOPTS ?= --leak-check=full --show-leak-kinds=all --error-exitcode=1 --errors-for-leak-kinds=all
endif


all: cursesklon test

cursesklon: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -vrf obj cursesklon testrunner

obj/%.o: src/%.c
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

.PHONY: iwyu
iwyu:
	$(MAKE) -f Makefile.iwyu

testrunner: $(TESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(TESTS_SRC) $(filter-out obj/main.o, $(OBJ)) -o testrunner $(LDFLAGS)

.PHONY: test
test: testrunner
	$(VALGRIND) $(VALGRINDOPTS) ./testrunner

-include $(DEPENDS)
