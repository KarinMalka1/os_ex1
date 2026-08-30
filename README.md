# User-Level Threads Library (uthreads)

A C++ implementation of a user-level (green) threads library, built as part of the
Hebrew University Operating Systems course (Exercise 1).

The library implements cooperative and preemptive multithreading entirely in user
space, using `sigsetjmp`/`siglongjmp` for context switching and `setitimer` with
signal handling to drive quantum-based preemption — no kernel threads involved.

## Features

- Thread creation, termination, blocking, and resuming
- Round-robin preemptive scheduling based on a configurable quantum length
- Thread sleeping (`uthread_sleep`) with automatic wake-up and re-queueing
- Per-thread and total quantum accounting

## Project layout

```
uthreads.h        Public API for the library
uthreads.cpp       Library implementation
tests/             Unit tests (.cpp) with expected output (.txt) and a Python test runner
demos/             Small standalone demos of the underlying OS primitives (itimer, sigsetjmp, signal handlers)
games/             Example programs (ants, thread_tron) built on top of the library
```

## Building

The library has no build system of its own — it's compiled together with whatever
program includes `uthreads.h`. For example:

```bash
g++ -Wall --std=c++17 -o my_program uthreads.cpp my_program.cpp
```

## Running the tests

Tests live in [tests/](tests/) as matched `.cpp`/`.txt` pairs and are compiled and run
via the included Python test runner:

```bash
python3 tests/run_tests.py
```

The runner compiles each test against `uthreads.cpp`, executes it, and compares its
output to the corresponding expected `.txt` file.

## Demos

[demos/](demos/) contains small, self-contained C programs illustrating the low-level
mechanisms the library relies on:

- `demo_itimer.c` — interval timers (`setitimer`) and signal-based timeouts
- `demo_jmp.c` — non-local jumps with `sigsetjmp`/`siglongjmp`
- `demo_singInt_handler.c` — installing a signal handler

## Games

[games/](games/) contains example programs built on top of the thread library to
exercise it in a more interactive way (`ants.cpp`, `thread_tron.cpp`).
