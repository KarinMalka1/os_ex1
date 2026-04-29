#include "uthreads.h"
#include <iostream>
#include <cassert>

void log(std::string msg) {
    std::cout << "[Quantum " << uthread_get_total_quantums() << "] " << msg << std::endl;
}

void sleeper() {
    int tid = uthread_get_tid();
    log("Thread " + std::to_string(tid) + " sleeping for 3 quantums.");
    uthread_sleep(3);
    log("Thread " + std::to_string(tid) + " woke up.");
    uthread_terminate(tid);
}

void complex_sleeper() {
    int tid = uthread_get_tid();
    log("Thread " + std::to_string(tid) + " (Complex) sleeping for 5 quantums.");
    uthread_sleep(5);
    log("Thread " + std::to_string(tid) + " (Complex) FINISHED sleep and is running.");
    uthread_terminate(tid);
}

int main() {
    if (uthread_init(10000) == -1) return 0;

    log("Main: Starting multiple sleep test.");
    int t1 = uthread_spawn(sleeper);
    int t2 = uthread_spawn(sleeper);
    (void)t1; // Silences the unused variable warning
    (void)t2;
    
    // Switch context to let t1 and t2 go to sleep
    uthread_sleep(0); 
    uthread_sleep(0);

    // Let time pass
    for(int i=0; i<4; ++i) uthread_sleep(0);

    log("Main: Starting Sleep + Block combo test.");
    int t3 = uthread_spawn(complex_sleeper);
    uthread_sleep(0); // t3 goes to sleep for 5

    log("Main: Blocking thread " + std::to_string(t3) + " while it is sleeping.");
    uthread_block(t3);

    // Run main for 10 quantums. T3 sleep should end, but it's blocked manually.
    for(int i=0; i<10; ++i) uthread_sleep(0);

    log("Main: Resuming thread " + std::to_string(t3) + ".");
    uthread_resume(t3); // T3 should enter ready queue now because sleep_remaining is 0

    uthread_sleep(0); // Switch to T3

    log("Main: Test completed.");
    uthread_terminate(0);
}