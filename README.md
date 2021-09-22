# Curses klondike

This is a text-based [klondike] game written in C using curses.

[klondike]: https://en.wikipedia.org/wiki/Klondike_(solitaire)

![screenshot](screenshot.png)

If you don't like colors, use the `--no-colors` option:

![screenshot](screenshot-nocolors.png)

If you have apt, you can install all dependencies like this:

    $ sudo apt install git make gcc libncurses5-dev libncursesw5-dev

Then you can download my code, compile it and run.

    $ git clone https://github.com/Akuli/curses-klondike
    $ cd curses-klondike
    $ make
    $ ./cursesklon


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
