Karin Malka, Victoria Koltan
214310740, 322228727
karin.malka@mail.huji.ac.il, victoria.koltan@mail.huji.ac.il
EX: 1

FILES:
README          - This file.
uthreads.cpp    - The main implementation of the user-level threads library.

REMARKS:
* Design Choices and Data Structures:
To manage the threads efficiently, we used several standard C++ containers:
- std::map<int, Thread*>: We used a map to keep track of all active threads by their IDs.
 This allows us to easily find, access, and verify the existence of any thread.

- std::deque<int>: We used a deque for the READY queue.
 It makes it very easy and efficient to pop the next thread from the front and push preempted or
 resumed threads to the back.

- std::queue<Thread*>: We used a queue for "zombie" threads. 
 When a thread terminates itself, it cannot delete its own stack while running.
 Instead, it pushes itself to the zombie queue, and the next thread that runs cleans it 
 up during the context switch.

* Memory Management:
To ensure no memory leaks occur, we implemented a global `GarbageCollector` struct. In scenarios where a test program ends simply by returning from `main` (meaning `uthread_terminate(0)` is never explicitly called), our garbage collector's destructor automatically runs at the end of the program. It iterates over the map and the zombie queue to free any remaining allocated stacks and Thread objects safely.

* Context Switching and Scheduling:
We implemented the Round-Robin scheduler using `setitimer` to trigger `SIGVTALRM` signals.
To avoid race conditions, we block signals using a custom RAII `SignalBlocker` struct 
whenever entering a critical library function, ensuring that the scheduler
doesn't interrupt a sensitive state change. Context saving and loading are done using `sigsetjmp` and `siglongjmp`.

ANSWERS:
None.