#include <iostream>
#include <cassert>
#include <vector>
#include "uthreads.h"

// Test 100: Thread order with sleep(0)
// sleep(0) should yield the CPU and move the thread to the end of the READY queue.

std::vector<int> execution_order;

void worker() {
    int tid = uthread_get_tid();
    execution_order.push_back(tid);
    uthread_terminate(tid);
}

int main() {
    uthread_init(100000);
    
    int t1 = uthread_spawn(worker);
    int t2 = uthread_spawn(worker);
    int t3 = uthread_spawn(worker);
    
    // Expected READY queue: [1, 2, 3]
    uthread_sleep(0); // T0 yields. New RUNNING: T1. Queue: [2, 3, 0(sleeping 0? no, wait, sleep(0) acts as yield)]
    
    // When T0 comes back, T1, T2, T3 should have finished
    assert(execution_order.size() == 3);
    assert(execution_order[0] == t1);
    assert(execution_order[1] == t2);
    assert(execution_order[2] == t3);
    
    std::cout << "Test 100 Passed!" << std::endl;
    uthread_terminate(0);
    return 0;
}
