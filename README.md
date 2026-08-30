# uthreads - OS Exercise 1

This is our implementation of a user-level threads library for the OS course exercise 1.
Basically we're building our own mini thread scheduler from scratch, without using real
kernel threads - just `sigsetjmp`/`siglongjmp` to jump between "threads" and a timer
signal (`setitimer`) that fires every quantum to force a switch. It's a bit mind-bending
at first but it clicks once you see it running.

## What it does

- create / terminate / block / resume threads
- round robin scheduling, each thread gets a quantum before it's preempted
- threads can sleep for N quantums and get woken back up automatically
- keeps track of how many quantums each thread ran for + total quantums overall

## Files

- `uthreads.h` / `uthreads.cpp` - the actual library, this is the part we wrote
- `tests/` - our test cases + a python script that compiles and runs them all and diffs
  the output against what's expected
- `demos/` - some small demo programs (not ours, given in the course) showing the raw
  building blocks: timers, sigsetjmp, signal handlers
- `games/` - a couple of little games we made just for fun to stress test the library
  (ants, thread_tron)

## How to compile

There's no makefile, just compile it together with whatever main file you're using:

```bash
g++ -Wall --std=c++17 -o my_program uthreads.cpp my_program.cpp
```

## Running the tests

```bash
python3 tests/run_tests.py
```

It compiles every test in `tests/` against our `uthreads.cpp` and checks the output
matches the `.txt` file next to it. Should all pass green if nothing's broken.

## Notes

Wrote and tested this on Linux (WSL), didn't check it on anything else. If something
looks weird with the stack address translation in `translate_address`, that's the
64-bit magic number trick from the course - don't touch it unless you know what you're
doing lol.
