#include <iostream>
#include <cassert>
#include "uthreads.h"

void thread2_func() {
    while (true) {
        uthread_sleep(0);
    }
}

void thread1_func() {
    int t2 = uthread_spawn(thread2_func);
    
    uthread_block(t2);
    
    for (int i = 0; i < 5; ++i) {
        uthread_sleep(0);
    }
    
    uthread_resume(t2);
    uthread_sleep(0); 
    
    uthread_block(t2);
    uthread_terminate(t2);

    uthread_block(uthread_get_tid());
    
    uthread_terminate(uthread_get_tid());
}

int main() {
    if (uthread_init(10000) == -1) {
        return 0;
    }

    uthread_block(0);
    uthread_block(999);
    uthread_resume(999);

    int t1 = uthread_spawn(thread1_func);
    
    for (int i = 0; i < 15; ++i) {
        uthread_sleep(0);
    }

    uthread_resume(t1);
    
    for (int i = 0; i < 5; ++i) {
        uthread_sleep(0);
    }

    std::cout << "Final total quantums: " << uthread_get_total_quantums() << std::endl;
    assert(uthread_get_total_quantums() >= 10);
    
    std::cout << "All Block/Resume tests passed!" << std::endl;
    
    uthread_terminate(0);
    return 0;
}