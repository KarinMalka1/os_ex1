#include <iostream>
#include <cassert>
#include "uthreads.h"

// Test 12: Wake between 2 previous
// T1 sleeps for 5 quantums. T2 sleeps for 15 quantums. T3 sleeps for 10 quantums.
// Expected wake order: T1, T3, T2

int order = 0;

void t1_func() {
    uthread_sleep(5);
    std::cout << "T1 woke up. Expected order 1, Actual: " << ++order << std::endl;
    assert(order == 1);
    uthread_terminate(uthread_get_tid());
}

void t2_func() {
    uthread_sleep(15);
    std::cout << "T2 woke up. Expected order 3, Actual: " << ++order << std::endl;
    assert(order == 3);
    uthread_terminate(uthread_get_tid());
}

void t3_func() {
    uthread_sleep(10);
    std::cout << "T3 woke up. Expected order 2, Actual: " << ++order << std::endl;
    assert(order == 2);
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(10000); // 10ms
    uthread_spawn(t1_func);
    uthread_spawn(t2_func);
    uthread_spawn(t3_func);
    
    while(order < 3) {
        uthread_sleep(0); // Yield to let other threads run
    }
    
    std::cout << "Test 12 Passed!" << std::endl;
    uthread_terminate(0);
    return 0;
}
