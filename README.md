# Curses solitaire

This is my attempt at making a solitaire game. It's challenging because:

- I have some experience with programming in C and programming with curses, but
  this is the second project where I use C *and* curses.
- I have attempted to make a solitaire game a few times in the past. It has
  always been too complicated to me.

Windows is not supported, but the game should work on most platforms when it's
ready.

If you have apt, you can install all dependencies like this:

    $ sudo apt install git make gcc libncursesw5-dev

Then you can download my code, compile it and run.

    $ git clone https://github.com/Akuli/curses-solitaire
    $ cd curses-solitaire
    $ make
    $ ./cursessol


## FAQ

### Why are you writing a solitaire game implementation?

Because it's fun.

### Why are you writing a solitaire game implementation in C?

Because it's fun.

### Why are you writing a solitaire game implementation using curses?

Because it's fun.

### Does it work on Windows?

No, but Windows comes with a solitaire. Windows command prompt and powershell
are kind of awful anyway, and you probably want to use GUI applications instead
of them whenever possible.
