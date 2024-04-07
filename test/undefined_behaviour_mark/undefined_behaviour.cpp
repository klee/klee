// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-no-tests --exit-on-error --external-calls=none %t1.bc

#include <iostream>
#include <thread>

const int NUM_THREADS = 2;
int counter = 0;

void incrementCounter() {
for (int i = 0; i < 1000000; ++i) {
counter++;
}
}

int main() {
std::thread threads[NUM_THREADS];

for (int i = 0; i < NUM_THREADS; ++i) {
threads[i] = std::thread(incrementCounter);
}

for (int i = 0; i < NUM_THREADS; ++i) {
threads[i].join();
}

std::cout << "Final counter value: " << counter << std::endl;

return 0;
} 
