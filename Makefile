CFLAGS += -Wall -Wextra -Wpedantic -std=gnu99 -Wno-unused-parameter
LDFLAGS += -lncursesw     # needs cursesw instead of curses for unicodes
IWYU ?= iwyu

SRC := $(filter-out src/main.c, $(wildcard src/*.c))
OBJ := $(SRC:src/%.c=obj/%.o)
HEADERS := $(wildcard src/*.h)

all: cursessol

cursessol: src/main.c $(OBJ) $(HEADERS)
	$(CC) -I. $(CFLAGS) $< $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -vrf obj cursessol

obj/%.o: src/%.c $(HEADERS)
	mkdir -p $(@D) && $(CC) -c -o $@ $< $(CFLAGS)

iwyu:
	for file in $(SRC) src/main.c; do $(IWYU) $$file; done || true
