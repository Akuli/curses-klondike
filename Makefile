CFLAGS += -Wall -Wextra -Wpedantic -std=gnu99 -Wno-unused-parameter
LDFLAGS += -lncursesw     # needs cursesw instead of curses for unicodes
IWYU ?= iwyu

SRC := $(filter-out src/main.c, $(wildcard src/*.c))
OBJ := $(SRC:src/%.c=obj/%.o)
HEADERS := $(wildcard src/*.h)
TESTS_SRC := $(wildcard tests/*.c)

# valgrind is used in tests
ifdef VALGRIND
VALGRINDOPTS ?= -q --leak-check=full --error-exitcode=1 --errors-for-leak-kinds=all
endif


all: test cursessol

cursessol: src/main.c $(OBJ) $(HEADERS)
	$(CC) -I. $(CFLAGS) $< $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -vrf obj cursessol testrunner

obj/%.o: src/%.c $(HEADERS)
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

.PHONY: iwyu
iwyu:
	for file in $(SRC) src/main.c; do $(IWYU) $$file; done || true
	for file in $(TESTS_SRC); do $(IWYU) -I. $$file; done || true

testrunner: $(TESTS_SRC) $(OBJ)
	$(CC) -I. $(CFLAGS) $(TESTS_SRC) $(OBJ) -o testrunner

.PHONY: test
test: testrunner
	$(VALGRIND) $(VALGRINDOPTS) ./testrunner
