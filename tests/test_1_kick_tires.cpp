#include "uthreads.h"
#include <iostream>


int main(int argc, char **argv) {
    int result = uthread_init(1000 * 1000);
    if (result != 0) {
        std::cout << "uthread_init failed with code " << result << std::endl;
        exit(-1);
    }
    int tid = uthread_get_tid();
    std::cout << "My thread ID: " << tid << std::endl;
}
