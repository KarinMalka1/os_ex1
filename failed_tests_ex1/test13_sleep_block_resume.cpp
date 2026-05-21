#include <iostream>
#include <cassert>
#include "uthreads.h"

// Test 13: Sleep, Block, and Resume
// A thread is sleeping, gets blocked, sleep expires, should not wake up until resumed.

bool t1_woke_up = false;

void t1_func() {
    uthread_sleep(2); // Sleep for 2 quantums
    t1_woke_up = true;
    std::cout << "T1 finished sleep and was resumed." << std::endl;
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(10000);
    int t1 = uthread_spawn(t1_func);
    uthread_sleep(0);
    // T1 is now sleeping. Let's block it.
    uthread_block(t1);
    
    // Wait for T1's sleep to definitely expire (e.g., 5 quantums)
    for(int i = 0; i < 5; i++) {
        uthread_sleep(0);
    }
    
    // If T1 woke up despite being blocked, the assertion would fail
    assert(!t1_woke_up && "Error: T1 woke up while blocked!");
    
    // Resume T1, it should now be moved to READY queue
    uthread_resume(t1);
    
    // Yield to let T1 run
    uthread_sleep(0);
    
    assert(t1_woke_up && "Error: T1 did not wake up after resume.");
    
    std::cout << "Test 13 Passed!" << std::endl;
    uthread_terminate(0);
    return 0;
}
