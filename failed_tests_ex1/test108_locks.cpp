#include <iostream>
#include <cassert>
#include "uthreads.h"

// Test 108: Locks in get_thread_quantums
// Checks if timer signals are properly masked during atomic operations.

void spam_quantums() {
    for(int i = 0; i < 1000000; i++) {
        uthread_get_quantums(0); 
    }
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(100); // Extremely short quantum to force timer signals
    
    uthread_spawn(spam_quantums);
    uthread_spawn(spam_quantums);
    uthread_spawn(spam_quantums);
    
    for(int i = 0; i < 10; i++) {
        uthread_get_quantums(0);
        uthread_sleep(0);
    }
    
    std::cout << "Test 108 Passed! No race condition crashes detected." << std::endl;
    uthread_terminate(0);
    return 0;
}
