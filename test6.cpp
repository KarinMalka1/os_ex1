#include "uthreads.h"
#include <iostream>

void thread1() {
    for (int i = 0; i < 3; ++i) {
        std::cout << "Thread 1 is running, quantum: " << uthread_get_quantums(uthread_get_tid()) << std::endl;
        uthread_sleep(1); // נכנס לשינה כדי להעביר תור
    }
    std::cout << "Thread 1 finishing..." << std::endl;
    uthread_terminate(uthread_get_tid());
}

void thread2() {
    for (int i = 0; i < 3; ++i) {
        std::cout << "Thread 2 is running, quantum: " << uthread_get_quantums(uthread_get_tid()) << std::endl;
        uthread_sleep(1);
    }
    std::cout << "Thread 2 finishing..." << std::endl;
    uthread_terminate(uthread_get_tid());
}

int main() {
    // אתחול עם קוונטום של 100 מילי-שניות (100,000 מיקרו)
    if (uthread_init(100000) != 0) {
        return 1;
    }

    uthread_spawn(thread1);
    uthread_spawn(thread2);

    std::cout << "Main thread waiting for others..." << std::endl;
    
    // לולאה כדי שה-Main לא יסתיים מיד (זה יסגור את כל התהליך)
    while (uthread_get_total_quantums() < 10) {
        // המערכת תבצע Context Switch לבד בזכות הטיימר
    }

    std::cout << "Test finished. Total quantums: " << uthread_get_total_quantums() << std::endl;
    uthread_terminate(0);
    return 0;
}