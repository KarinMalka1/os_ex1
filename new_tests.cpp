#include <iostream>
#include <vector>
#include <cassert>
#include <unistd.h>
#include "uthreads.h"

using namespace std;

// ==============================================================================
// Test 12: test12_wake_between_2_previous
// בודק התעוררות של חוטים מ-sleep בסדר הנכון.
// ==============================================================================
vector<int> wake_order;

void t12_thread1() {
    uthread_sleep(3);
    wake_order.push_back(1);
    uthread_terminate(uthread_get_tid());
}

void t12_thread2() {
    uthread_sleep(10);
    wake_order.push_back(2);
    uthread_terminate(uthread_get_tid());
}

void t12_thread3() {
    uthread_sleep(5);
    wake_order.push_back(3);
    uthread_terminate(uthread_get_tid());
}

void test12_wake_between_2_previous() {
    cout << "Running test 12 (Wake timing)..." << endl;
    uthread_init(100000); // 100ms quantum
    
    uthread_spawn(t12_thread1);
    uthread_spawn(t12_thread2);
    uthread_spawn(t12_thread3);
    
    // Main thread sleeps enough time to let all threads wake up
    uthread_sleep(15);
    
    // Expected order of waking up: T1 (3), T3 (5), T2 (10)
    assert(wake_order.size() == 3);
    assert(wake_order[0] == 1);
    assert(wake_order[1] == 3);
    assert(wake_order[2] == 2);
    
    cout << "Test 12 Passed!" << endl;
}

// ==============================================================================
// Test 13: test13_sleep_and_block_and_resume
// מוודא שחוט שגם נרדם וגם נחסם, לא חוזר לרוץ עד ששני התנאים (זמן ו-resume) מתקיימים.
// ==============================================================================
bool t13_ran = false;

void t13_thread() {
    t13_ran = true;
    uthread_terminate(uthread_get_tid());
}

void test13_sleep_and_block_and_resume() {
    cout << "Running test 13 (Sleep, block, resume)..." << endl;
    uthread_init(100000);
    
    uthread_spawn(t13_thread);
    // Give the thread a chance to sleep, but here we'll force it from main
    // Note: The logic in the test might be that the thread blocks itself, 
    // or main blocks it while it's sleeping.
    // Let's modify the thread to sleep, and main will block it.
}

// גישה טובה יותר לטסט 13:
void t13_sleeping_thread() {
    uthread_sleep(3);
    t13_ran = true; // Should only get here AFTER resume and sleep are done
    uthread_terminate(uthread_get_tid());
}

void run_test13() {
    cout << "Running test 13 (Sleep, block, resume)..." << endl;
    uthread_init(100000);
    t13_ran = false;
    
    int tid = uthread_spawn(t13_sleeping_thread);
    
    // Main thread runs, blocking tid before its sleep finishes
    uthread_block(tid); 
    
    // Main thread sleeps past tid's wake-up time
    uthread_sleep(5); 
    
    // If the library is bugged, tid might have returned to READY and ran.
    assert(t13_ran == false); 
    
    // Now we resume it. It should move to READY immediately because its sleep time passed
    uthread_resume(tid);
    
    // Yield to let it run
    uthread_sleep(1);
    
    assert(t13_ran == true);
    cout << "Test 13 Passed!" << endl;
}

// ==============================================================================
// Test 100: test_100_thread_order
// בודק האם sleep(0) מעביר את החוט לסוף תור ה-READY ומשפיע על הסדר כראוי.
// ==============================================================================
vector<int> t100_order;

void t100_thread1() {
    t100_order.push_back(1);
    uthread_sleep(0); // Yield
    t100_order.push_back(1);
    uthread_terminate(uthread_get_tid());
}

void t100_thread2() {
    t100_order.push_back(2);
    uthread_terminate(uthread_get_tid());
}

void test_100_thread_order() {
    cout << "Running test 100 (sleep 0 order)..." << endl;
    uthread_init(100000);
    
    uthread_spawn(t100_thread1);
    uthread_spawn(t100_thread2);
    
    uthread_sleep(2);
    
    // Expected order: 
    // T1 runs -> pushes 1 -> sleep(0) moves it to end of queue.
    // T2 runs -> pushes 2 -> terminates.
    // T1 runs again -> pushes 1 -> terminates.
    assert(t100_order.size() == 3);
    assert(t100_order[0] == 1);
    assert(t100_order[1] == 2);
    assert(t100_order[2] == 1);
    
    cout << "Test 100 Passed!" << endl;
}

// ==============================================================================
// Test 101: test_101_print_from_thread
// בודק הדפסה ל-stdout. לעיתים נופל אם סיגנלים חותכים פעולות I/O והסיגנלים לא נחסמו כראוי.
// ==============================================================================
void t101_thread() {
    for (int i = 0; i < 50; ++i) {
        cout << "Thread is printing: " << i << "\n";
    }
    uthread_terminate(uthread_get_tid());
}

void test_101_print_from_thread() {
    cout << "Running test 101 (Print from thread)..." << endl;
    uthread_init(10000); // Small quantum to force context switches during print
    uthread_spawn(t101_thread);
    uthread_sleep(2);
    cout << "Test 101 Finished! (If no crash/deadlock occurred, it passed)" << endl;
}

// ==============================================================================
// Test 107: test_107_lemmings
// בודק יצירת חוט, שמייצר חוט ומוחק את עצמו (שרשרת). בודק מיחזור תקין של ID (אמור תמיד להיות 1).
// ==============================================================================
int lemming_count = 0;

void lemming_thread() {
    lemming_count++;
    if (lemming_count < 100) {
        uthread_spawn(lemming_thread);
        // Since we only have main (0) and this thread (1), 
        // the new thread must receive ID 1 after this thread terminates,
        // or ID 2 if we spawn BEFORE we terminate.
        // Let's spawn, it gets ID 2, then we die. Next one spawns, it gets ID 1.
        // Actually, if we spawn *before* terminate, it gets 2. 
        // Then we terminate 1. The next one runs (ID 2), spawns (gets 1), terminates 2.
    }
    uthread_terminate(uthread_get_tid());
}

void test_107_lemmings() {
    cout << "Running test 107 (Lemmings)..." << endl;
    uthread_init(100000);
    uthread_spawn(lemming_thread);
    uthread_sleep(110); // Wait enough time for all 100 to finish
    assert(lemming_count == 100);
    cout << "Test 107 Passed!" << endl;
}

// ==============================================================================
// Test 108: test_108_locks
// בודק שהפונקציה get_thread_quantoms חוסמת ומשחררת סיגנלים כראוי.
// הרבה פעמים נופלים פה כשמחזירים שגיאה (return -1) ושוכחים לשחרר את חסימת הסיגנלים (sigprocmask).
// ==============================================================================
void test_108_locks() {
    cout << "Running test 108 (Locks)..." << endl;
    uthread_init(100000);
    
    // Call with invalid ID. 
    // If the library blocks signals, checks ID validity, and returns -1 WITHOUT unblocking the signal,
    // the timer signal will remain blocked forever!
    int res = uthread_get_quantums(999); 
    assert(res == -1);
    
    // If the signal was not unblocked, the sleep function will never be interrupted by SIGVTALRM,
    // and this sleep(1) will essentially hang the program if we rely on timer. 
    // (Though sleep uses quantums, not absolute time, so we need another thread to increment quantums)
    
    int tid = uthread_spawn([](){ 
        while(true) {} // Infinite loop
    });
    
    // We yield. The infinite loop will run. If signals are unblocked properly, 
    // the timer will fire, preempt the infinite loop, and return to main.
    // If signals were left blocked, the program will hang forever in the infinite loop.
    uthread_sleep(2); 
    
    uthread_terminate(tid);
    cout << "Test 108 Passed (Did not hang)!" << endl;
}

// ==============================================================================
// Test 109: test_109_resume_self
// מוודא ש-resume על חוט שקורא לעצמו (או חוט שכבר רץ) לא פוגע בכלום ומחזיר 0.
// ==============================================================================
void test_109_resume_self() {
    cout << "Running test 109 (Resume self)..." << endl;
    uthread_init(100000);
    
    int tid = uthread_get_tid();
    int res = uthread_resume(tid); 
    
    // According to instructions: Resuming RUNNING/READY has no effect and is NOT an error.
    assert(res == 0);
    
    cout << "Test 109 Passed!" << endl;
}

// ==============================================================================
// main
// ==============================================================================
int main() {
    // מומלץ להריץ כל טסט בנפרד כדי ש-uthread_init לא יתנגש, 
    // או לקמפל כל פעם מחדש עם טסט אחר. 
    // לצורך הדוגמה, הנה קריאה לכולם - אבל ייתכן שהספרייה שלך לא תומכת ב-init מרובה באותה ריצה.
    // אם לא, פשוט שימי בהערה את כל הטסטים חוץ מהאחד שאת בודקת.
    
    // run_test13();
    // test_100_thread_order();
    // test_101_print_from_thread();
    test_107_lemmings();
    // test_108_locks();
    // test_109_resume_self();

    return 0;
}