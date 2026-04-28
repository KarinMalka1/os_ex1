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
    uthread_sleep(0); // מעבר ל-T2
    uthread_terminate(t2); // מחיקה חיצונית
    uthread_terminate(uthread_get_tid()); // מחיקה עצמית
}

int main() {
    uthread_init(10000);
    uthread_spawn(thread1_func);
    
    uthread_sleep(0); // עוברים ל-T1
    
    // כשאנחנו כאן, T1 ו-T2 כבר נמחקו
    std::cout << "Final Quantums: " << uthread_get_total_quantums() << std::endl;
    std::cout << "Test 3.5 Passed Successfully!" << std::endl;
    
    uthread_terminate(0);
    return 0;
}