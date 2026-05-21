#include <iostream>
#include "uthreads.h"

// 1. משתנה בוליאני שיעזור לנו לדעת אם החוט סיים בהצלחה
bool test_passed = false;

// Test 109: Resume self
// Resuming itself should do nothing.

void resume_self() {
    int my_tid = uthread_get_tid();
    int result = uthread_resume(my_tid);
    
    // The instructions say "A thread can block itself (but not resume itself, obviously)." 
    // Depending on implementation, it might return -1 or 0, but it MUST NOT crash or duplicate in READY queue.
    
    uthread_sleep(0); // Yield to see if duplicate exists
    
    std::cout << "Test 109 Passed! Result of resume self: " << result << std::endl;
    
    // 2. סימון שהטסט עבר בהצלחה
    test_passed = true;
    
    uthread_terminate(my_tid);
}

int main() {
    uthread_init(100000);
    uthread_spawn(resume_self);
    
    // 3. נותנים לחוט השני כמה הזדמנויות לרוץ ולסיים (במקום פעם אחת)
    for(int i = 0; i < 3; i++) {
        uthread_sleep(0); 
    }
    
    // 4. אם החוט לא הספיק לעדכן את הדגל, סימן שהוא נתקע או שהקוד לא עבד כמו שצריך
    if (!test_passed) {
        std::cerr << "Test 109 Failed! The thread did not complete successfully." << std::endl;
    }
    
    uthread_terminate(0);
    return 0;
}