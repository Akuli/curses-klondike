# Curses klondike

This is a text-based [klondike](https://en.wikipedia.org/wiki/Klondike_(solitaire)) game
written in [my Jou programming language](https://github.com/Akuli/jou) using curses.

![screenshot](screenshot.png)

If you don't like colors, use the `--no-colors` option:

![screenshot](screenshot-nocolors.png)


## Setup

1. Install Jou version 2025-04-29-0400 using [Jou's instructions](https://github.com/Akuli/jou/blob/2025-04-29-0400/README.md#setup).
    Newer versions of Jou may also work.
2. Install curses:
    ```
    sudo apt install libncurses5-dev libncursesw5-dev
    ```
3. Download and compile the code:
    ```
    git clone https://github.com/Akuli/curses-klondike
    ```
4. Compile and run:
    ```
    cd curses-klondike
    jou src/main.jou
    ```


## FAQ

### How do I exit this game?

It's not vim, so you can quit it like any other sane curses program (less,
bsdgames tetris etc). In other words, press q to quit.

### I don't like the rules that this game uses! When X happens, it should do Y instead of Z

See `./cursesklon --help`. Maybe one of the options is what you want. If none
of them is, [create an issue].

[create an issue]: https://github.com/Akuli/curses-klondike/issues/new

### Why did you write a klondike game?

Because it's fun.

### Why did you write a klondike game using curses?

Because it's fun.

### Does it work on Windows?

No, but Windows comes with a klondike. Windows command prompt and powershell
are kind of awful anyway, and you probably want to use GUI applications instead
of them whenever possible.
