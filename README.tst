Karin Malka, Victoria Koltan
214310740, 322228727
karin.malka@mail.huji.ac.il, victoria.koltan@mail.huji.ac.il
EX: 1

FILES:
README          - This file.
uthreads.cpp    - The main implementation of the user-level threads library.

REMARKS:
* Design Choices and Data Structures:
To manage the threads efficiently and meet the requirement of finding the smallest 
available ID, we used the following standard C++ containers:

- std::map<int, Thread*>: 
Used to keep track of all active threads by their IDs. This allows for O(log n) 
access, insertion, and deletion, ensuring we can easily verify if a thread 
exists and access its context.

- std::deque<int>: 
Used for the READY queue[cite: 71]. A deque was chosen because it provides 
efficient O(1) operations for popping from the front (scheduling) and 
pushing to the back (preemption/resuming).

- std::queue<Thread*>: 
Used as a "zombie" queue for threads that have terminated themselves[cite: 59, 60]. 
Since a running thread cannot safely delete its own stack while it is still 
in use, it is marked for deletion and added to this queue. The next thread 
selected by the scheduler handles the actual memory deallocation of the 
zombie thread's resources.

* Memory Management:
To prevent memory leaks and ensure robust cleanup[cite: 80], we implemented a 
global `GarbageCollector` struct. This acts as an RAII-based manager that 
is particularly useful if the program ends without an explicit call to 
terminate the main thread. Its destructor iterates through all remaining 
threads in the map and the zombie queue to free allocated stacks and 
Thread objects.

* Context Switching and Scheduling:
We implemented the Round-Robin (RR) scheduling algorithm  using the 
Virtual Timer (`ITIMER_VIRTUAL`) to trigger `SIGVTALRM` signals. 
- Signal Safety: To prevent race conditions during critical sections (like 
  modifying the READY queue), we implemented a `SignalBlocker` RAII struct 
  that uses `sigprocmask` to block/unblock signals automatically.
- Context Management: `sigsetjmp` and `siglongjmp` are used to save and 
  restore thread states. As required, we utilized the 
  `translate_address` function to correctly set the stack and instruction 
  pointers for new threads.

ANSWERS:
None.