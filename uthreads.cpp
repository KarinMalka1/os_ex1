#include "uthreads.h"
#include <setjmp.h>
#include <signal.h>
#include <iostream>
#include <deque>
#include <queue>
#include <map>
#include <algorithm> 

/* code for 64 bit Intel arch */
typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A helper function to translate addresses for sigsetjmp and siglongjmp.
   You must use this as seen in the demo  */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor %%fs:0x30,%0\n"
		"rol $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

// --- חלק 1: מבנים פנימיים שחבויים מהמשתמש --- [cite: 10, 11]
enum State { READY, RUNNING, BLOCKED };

struct Thread {
    int tid;
    State state;                // המצב הנוכחי של החוט [cite: 21]
    char* stack;                // מצביע למחסנית (למעט חוט 0) [cite: 75, 87]
    sigjmp_buf env;             // שמירת ה-Context (רגיסטרים וכו') [cite: 106]
    int quantums_count;         // כמה קוונטומים החוט רץ בסך הכל [cite: 175]
    bool forreal = false;
    int sleep_remaining=0;

    Thread(int id, State s) : tid(id), state(s), stack(nullptr), quantums_count(0) {}
};

// משתנים גלובליים לניהול המצב (או מחלקה שתכיל אותם)
std::deque<int> ready_queue; // תור לניהול READY [cite: 71, 191]
int total_quantums_counter = 0; // מונה קוונטומים כללי [cite: 175]
Thread* running_thread = nullptr;
std::map<int, Thread*> all_threads;
std::queue<Thread*> zombie_queue;

/**
 * @brief initializes the thread library.
 *
 * Once this function returns, the main thread (tid == 0) will be set as RUNNING. There is no need to 
 * provide an entry_point or to create a stack for the main thread - it will be using the "regular" stack and PC.
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if (quantum_usecs<=0){
        std::cerr << "thread library error: " << "did not implement" << std::endl;
        return -1;
    }
    total_quantums_counter = 1;
    running_thread = new Thread(0, RUNNING); // הכל הוגדר בשורה אחת!
    (*running_thread).quantums_count = 1;
    all_threads[0]= running_thread;
    return 0;
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 * It is an error to call this function with a null entry_point.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point) {
    
    if (entry_point==nullptr){
        std::cerr << "thread library error: " << "entry_point is null" << std::endl;
        return -1;
    }
    int new_id = -1;
    for (int i = 1; i < MAX_THREAD_NUM; ++i) {
        if (all_threads.find(i) == all_threads.end()) {
            new_id = i;
            break;
        }
    }

    if (new_id == -1) {
        std::cerr << "thread library error: reached max threads limit\n";
        return -1;
    }

    // 3. יצירת החוט והקצאת מחסנית
    Thread* thread = new Thread(new_id, READY);
    try {
        thread->stack = new char[STACK_SIZE];
    } catch (const std::bad_alloc& e) {
        std::cerr << "system error: stack allocation failed\n";
        exit(1);
    }


    address_t sp = (address_t)thread->stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t)entry_point;

    sigsetjmp(thread->env, 1);
    (thread->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (thread->env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&thread->env->__saved_mask);

    // 5. הוספה למערכת
    all_threads[new_id] = thread;
    ready_queue.push_back(new_id); // אנחנו דוחפים את ה-ID לתור ה-int

    return new_id;
}

void cleanup_zombie(Thread* t) {
    if (t) {
        if (t->stack) {
            delete[] t->stack;
        }
        delete t;
    }
}

//context switch
void scheduler(){
    for (auto const& [tid, thread] : all_threads) {
        if (thread->sleep_remaining > 0){
            thread->sleep_remaining--;
        }
        if (thread->sleep_remaining == 0 && !thread->forreal) {
                thread->state = READY;
                ready_queue.push_back(tid);
        }
    }
    if (running_thread != nullptr) {
        if (sigsetjmp(running_thread->env, 1) != 0) {
            while (!zombie_queue.empty()) {
                Thread* zombie = zombie_queue.front();
                zombie_queue.pop();
                cleanup_zombie(zombie);
            }
            return; // ממשיכים בחיים של החוט שהתעורר
        }
        
    }

    if (ready_queue.empty()) {
        return; 
    }

    int next_tid = ready_queue.front();
    ready_queue.pop_front();
    running_thread = all_threads[next_tid];

    running_thread->state = RUNNING;
    running_thread -> quantums_count++;
    total_quantums_counter++;

    siglongjmp(running_thread->env, 1);
}

/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " does not exist\n";
        return -1;
    }

    if (tid == 0) {
        // ניקוי של הכל ויציאה מהתוכנית
        for (auto const& [id, t] : all_threads) {
            if (id != 0) delete[] t->stack;
            delete t;
        }
        all_threads.clear();
        exit(0);
    }

    Thread* t = all_threads[tid];
    
    // הסרה מהמפה כדי שאיש לא יוכל לעשות לו block/resume יותר
    all_threads.erase(tid);

    // הסרה מה-ready_queue (אם הוא היה שם)
    auto it = std::find(ready_queue.begin(), ready_queue.end(), tid);
    if (it != ready_queue.end()) {
        ready_queue.erase(it);
    }

    // הבדיקה הקריטית: האם החוט מוחק את עצמו?
    if (uthread_get_tid() == tid) {
        zombie_queue.push(t); // נשלח לניקוי ע"י החוט הבא
        running_thread = nullptr; // מסמנים ל-scheduler לא לשמור context
        scheduler();
        // לא נגיע לכאן לעולם
    } else {
        // מחיקת חוט אחר - בטוח לנקות עכשיו
        cleanup_zombie(t);
    }

    return 0;
}


/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is *not* considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " not found" << std::endl;
        return -1;
    }

    if (tid == 0) {
        std::cerr << "thread library error: cannot block main thread" << std::endl;
        return -1;
    }

    all_threads[tid]->state=BLOCKED; 
    all_threads[tid]->forreal = true;

    if (running_thread->tid == tid) {
        scheduler();
        return 0;
    }
    auto it = std::find(ready_queue.begin(), ready_queue.end(), tid);
    if (it != ready_queue.end()) {
        ready_queue.erase(it);
    }
    return 0;//found the tid
}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * When a thread transition to the READY state it is placed at the end of the READY queue.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " not found" << std::endl;
        return -1;
    }
    all_threads[tid]->forreal = false; 

    if (all_threads[tid]->state == BLOCKED && all_threads[tid]->sleep_remaining == 0){
        all_threads[tid]->state=READY; 
        ready_queue.push_back(tid);
    }
    return 0;
}


/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY queue.
 * If the thread which was just RUNNING should also be added to the READY queue, or if multiple threads wake up 
 * at the same time, the order in which they're added to the end of the READY queue doesn't matter.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * A call with num_quantums == 0 will immediately stop the thread and move it to the back of the execution queue.
 * 
 * It is considered an error if the main thread (tid == 0) calls this function with num_quantums != 0.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums) {
    if(running_thread->tid == 0 && num_quantums != 0){
        std::cerr << "thread library error: main thread cannot sleep\n";
        return -1;
    }
    if (num_quantums == 0){
        running_thread->state = READY;
        ready_queue.push_back(uthread_get_tid());//we moved current running thread ti the end of the queue of all the ready threads
        scheduler();
        return 0;
    }
    //num_quantums>0
    running_thread->sleep_remaining = num_quantums;
    running_thread->state = BLOCKED;
    scheduler();
    return 0;
}


/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid() {
    return running_thread->tid;
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums() {
    
    return total_quantums_counter;
}


/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    if (all_threads.find(tid) == all_threads.end()) {
        std::cerr << "thread library error: thread " << tid << " does not exist\n";
        return -1;
    }
    return all_threads[tid]->quantums_count;
}


