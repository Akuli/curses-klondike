# Curses klondike

This is a text-based [klondike] game written in curses using C.

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

### Why did you write a klondike game?

Because it's fun.

### Why did you write a klondike game in C?

Because it's fun.

### Why did you write a klondike game using curses?

Because it's fun.

### Does it work on Windows?

No, but Windows comes with a klondike. Windows command prompt and powershell
are kind of awful anyway, and you probably want to use GUI applications instead
of them whenever possible.
