#include <iostream>
#include "uthreads.h"

// Test 101: std::cout from thread
// Validates that IO doesn't cause crashes, potentially checking if signals are blocked correctly during IO if required by underlying OS/system (though standard cout usually just works, sometimes test checks for proper state maintenance).

void print_func() {
    std::cout << "Hello from thread " << uthread_get_tid() << std::endl;
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(100000);
    uthread_spawn(print_func);
    uthread_sleep(0); // Yield to let thread print
    std::cout << "Test 101 Passed!" << std::endl;
    uthread_terminate(0);
    return 0;
}
