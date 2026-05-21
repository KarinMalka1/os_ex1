#include <iostream>
#include <cassert>
#include "uthreads.h"

// Test 107: Lemmings
// Chain of threads spawning the next and terminating themselves.
// Tests ID reuse and thread cleanup limit.

const int MAX_LEMMINGS = 150; // Larger than max threads to ensure ID reuse
int lemmings_count = 0;

void lemming() {
    lemmings_count++;
    int my_tid = uthread_get_tid();
    
    if (lemmings_count < MAX_LEMMINGS) {
        int next_tid = uthread_spawn(lemming);
        assert(next_tid > 0 && "Spawn failed, possibly max threads not managed correctly");
    }
    
    uthread_terminate(my_tid);
}

int main() {
    uthread_init(10000);
    uthread_spawn(lemming);
    
    // Wait for lemmings
    while(lemmings_count < MAX_LEMMINGS) {
        uthread_sleep(0);
    }
    
    std::cout << "Test 107 Passed! Created " << lemmings_count << " lemmings sequentially." << std::endl;
    uthread_terminate(0);
    return 0;
}
