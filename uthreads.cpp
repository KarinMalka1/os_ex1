#include "uthreads.h"
#include <setjmp.h>
#include <signal.h>
#include <iostream>
#include <deque>
#include <queue>
#include <map>
#include <algorithm>
#include <sys/time.h>
#include <vector>
/* code for 64 bit Intel arch */
typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7
#define QUANTA 1000000

/**
 * @brief A helper function to translate addresses for sigsetjmp and siglongjmp.
 */
address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor %%fs:0x30,%0\n"
                 "rol $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

// --- Internal Structures ---
enum State { READY, RUNNING, BLOCKED };

struct Thread {
    int tid;
    State state;
    char* stack;
    sigjmp_buf env;
    int quantums_count;
    bool forreal = false;
    int sleep_remaining = 0;
    thread_entry_point entry_point;

    Thread(int id, State s, thread_entry_point ep = nullptr) 
        : tid(id), state(s), stack(nullptr), quantums_count(0), entry_point(ep) {}
};

// Global management variables
sigset_t signal_set;
std::deque<int> ready_queue;
int total_quantums_counter = 0;
Thread* running_thread = nullptr;
std::map<int, Thread*> all_threads;
std::queue<Thread*> zombie_queue;
struct itimerval global_timer;

/**
 * @brief Blocks signals for RAII-based thread safety.
 */
struct SignalBlocker {
    SignalBlocker() {
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGVTALRM);
        sigprocmask(SIG_BLOCK, &signal_set, NULL);
    }
    ~SignalBlocker() {
        sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
    }
};

void cleanup_zombie(Thread* t) {
    if (t) {
        if (t->stack) {
            delete[] t->stack;
        }
        delete t;
    }
}

/**
 * @brief The Scheduler function - handles context switching.
 */
void scheduler(int sig) {
    std::vector<int> threads_to_wake;
    // Handle sleeping threads decrement
    // if (sig == SIGVTALRM) {
        for (auto const& pair : all_threads) {
            int tid = pair.first;
            Thread* thread = pair.second;
            
            if (thread->sleep_remaining > 0) {
                thread->sleep_remaining--;
            }
            if (thread->sleep_remaining == 0 && !thread->forreal && thread->state == BLOCKED) {
                // thread->state = READY;
                threads_to_wake.push_back(tid);
            }
        }
    // }

    // Save current context
    if (running_thread != nullptr) {
        if (sigsetjmp(running_thread->env, 1) != 0) {
            // This part is executed when we return to the thread
            while (!zombie_queue.empty()) {
                Thread* zombie = zombie_queue.front();
                zombie_queue.pop();
                cleanup_zombie(zombie);
            }
            return;
        }

        if (sig == SIGVTALRM) {
            running_thread->state = READY;
            ready_queue.push_back(running_thread->tid);
        }
    }
    for (int tid : threads_to_wake) {
        all_threads[tid]->state = READY;
        ready_queue.push_back(tid);
    }
    if (ready_queue.empty()) {
        std::cerr << "system error: ready queue is empty (deadlock)\n";
        exit(1);
    }

    // Pick next thread
    int next_tid = ready_queue.front();
    ready_queue.pop_front();
    
    running_thread = all_threads[next_tid];
    running_thread->state = RUNNING;
    running_thread->quantums_count++;
    total_quantums_counter++;

    // Reset the timer since a new quantum begins!
    if (setitimer(ITIMER_VIRTUAL, &global_timer, NULL)) {
        std::cerr << "system error: setitimer failed" << std::endl;
        exit(1);
    }

    siglongjmp(running_thread->env, 1);
}

/**
 * @brief Wrapper for new threads to handle cleanup, execution, and safe termination.
 */
void start_thread() {
    // Unblock timer signal (since siglongjmp jumps here, it won't be unblocked by returning from interrupt)
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGVTALRM);
    sigprocmask(SIG_UNBLOCK, &set, NULL);

    // Clean zombies that were left before we jumped into this brand new thread
    while (!zombie_queue.empty()) {
        Thread* zombie = zombie_queue.front();
        zombie_queue.pop();
        cleanup_zombie(zombie);
    }

    // Run the actual thread function
    if (running_thread && running_thread->entry_point) {
        running_thread->entry_point();
    }

    // If the thread function ever returns, gracefully terminate it
    uthread_terminate(uthread_get_tid());
}

int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= 0) {
        std::cerr << "thread library error: quantum must be positive" << std::endl;
        return -1;
    }

    // Configure Timer Signal
    struct sigaction sa = {0};
    sa.sa_handler = &scheduler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0) {
        std::cerr << "system error: sigaction failed" << std::endl;
        exit(1);
    }

    // Configure Timer globally
    long sec = quantum_usecs / QUANTA;
    long usec = quantum_usecs % QUANTA;

    global_timer.it_value.tv_sec = sec;
    global_timer.it_value.tv_usec = usec;
    global_timer.it_interval.tv_sec = sec;
    global_timer.it_interval.tv_usec = usec;

    if (setitimer(ITIMER_VIRTUAL, &global_timer, NULL)) {
        std::cerr << "system error: setitimer failed" << std::endl;
        return -1;
    }

    // Initialize main thread
    total_quantums_counter = 1;
    running_thread = new Thread(0, RUNNING);
    running_thread->quantums_count = 1;
    all_threads[0] = running_thread;

    return 0;
}

int uthread_spawn(thread_entry_point entry_point) {
    SignalBlocker blocker;
    if (entry_point == nullptr) {
        std::cerr << "thread library error: entry_point is null" << std::endl;
        return -1;
    }

    int new_id = -1;
    for (int i = 1; i < MAX_THREAD_NUM; ++i) {
        if (all_threads.find(i) == all_threads.end()) {
            new_id = i;
            break;
        }
    }

    if (new_id == -1) {
        std::cerr << "thread library error: reached max threads limit\n";
        return -1;
    }

    Thread* thread = new Thread(new_id, READY, entry_point);
    try {
        thread->stack = new char[STACK_SIZE];
    } catch (const std::bad_alloc& e) {
        std::cerr << "system error: stack allocation failed\n";
        exit(1);
    }

    address_t sp = (address_t)thread->stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t)start_thread; // Jump to wrapper, NOT entry_point!

    sigsetjmp(thread->env, 1);
    (thread->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (thread->env->__jmpbuf)[JB_PC] = translate_address(pc);

    all_threads[new_id] = thread;
    ready_queue.push_back(new_id);

    return new_id;
}

int uthread_terminate(int tid) {
    SignalBlocker blocker;
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " does not exist\n";
        return -1;
    }

    if (tid == 0) {
        // 1. Disable the timer entirely so scheduler doesn't interrupt exit process
        struct itimerval timer_off = {0};
        setitimer(ITIMER_VIRTUAL, &timer_off, NULL);

        int current_tid = uthread_get_tid();

        // 2. Clean zombies
        while (!zombie_queue.empty()) {
            Thread* zombie = zombie_queue.front();
            zombie_queue.pop();
            cleanup_zombie(zombie);
        }

        // 3. Delete all threads safely
        for (auto const& pair : all_threads) {
            int id = pair.first;
            Thread* t = pair.second;
            
            if (id != 0 && id != current_tid) {
                delete[] t->stack;
            }
            delete t;
        }
        all_threads.clear();
        exit(0);
    }

    Thread* t = all_threads[tid];
    all_threads.erase(tid);

    // Erase all instances securely
    ready_queue.erase(std::remove(ready_queue.begin(), ready_queue.end(), tid), ready_queue.end());

    if (uthread_get_tid() == tid) {
        zombie_queue.push(t);
        running_thread = nullptr;
        scheduler(0);
    } else {
        cleanup_zombie(t);
    }

    return 0;
}

int uthread_block(int tid) {
    SignalBlocker blocker;
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " not found" << std::endl;
        return -1;
    }

    if (tid == 0) {
        std::cerr << "thread library error: cannot block main thread" << std::endl;
        return -1;
    }

    all_threads[tid]->state = BLOCKED;
    all_threads[tid]->forreal = true;

    if (running_thread->tid == tid) {
        scheduler(0);
        return 0;
    }

    ready_queue.erase(std::remove(ready_queue.begin(), ready_queue.end(), tid), ready_queue.end());
    return 0;
}

int uthread_resume(int tid) {
    SignalBlocker blocker;
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " not found" << std::endl;
        return -1;
    }

    all_threads[tid]->forreal = false;

    if (all_threads[tid]->state == BLOCKED && all_threads[tid]->sleep_remaining == 0) {
        all_threads[tid]->state = READY;
        ready_queue.push_back(tid);
    }
    return 0;
}

int uthread_sleep(int num_quantums) {
    SignalBlocker blocker;
    if (running_thread->tid == 0 && num_quantums != 0) {
        std::cerr << "thread library error: main thread cannot sleep\n";
        return -1;
    }

    if (num_quantums == 0) {
        running_thread->state = READY;
        ready_queue.push_back(uthread_get_tid());
        scheduler(0);
        return 0;
    }

    running_thread->sleep_remaining = num_quantums+1;
    running_thread->state = BLOCKED;
    scheduler(0);
    return 0;
}

int uthread_get_tid() {
    return running_thread->tid;
}

int uthread_get_total_quantums() {
    return total_quantums_counter;
}

int uthread_get_quantums(int tid) {
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " does not exist\n";
        return -1;
    }
    return all_threads[tid]->quantums_count;
}