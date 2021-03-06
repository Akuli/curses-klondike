CFLAGS += -Wall -Wextra -Wpedantic -std=c99 -Wno-unused-parameter
LDFLAGS += -lncursesw     # needs cursesw instead of curses for unicodes
IWYU ?= iwyu

SRC := $(filter-out src/main.c, $(wildcard src/*.c))
OBJ := $(SRC:src/%.c=obj/%.o)
HEADERS := $(wildcard src/*.h)
TESTS_SRC := $(wildcard tests/*.c)

# valgrind is used in tests
ifdef VALGRIND
VALGRINDOPTS ?= --leak-check=full --show-leak-kinds=all --error-exitcode=1 --errors-for-leak-kinds=all
endif


all: test cursesklon

cursesklon: src/main.c $(OBJ) $(HEADERS)
	$(CC) $(CFLAGS) $< $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -vrf obj cursesklon testrunner

obj/%.o: src/%.c $(HEADERS)
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

.PHONY: iwyu
iwyu:
	for file in $(SRC) src/main.c; do $(IWYU) $$file; done || true
	for file in $(TESTS_SRC); do $(IWYU) -I. $$file; done || true

testrunner: $(TESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(TESTS_SRC) $(OBJ) -o testrunner $(LDFLAGS)

.PHONY: test
test: testrunner
	$(VALGRIND) $(VALGRINDOPTS) ./testrunner
