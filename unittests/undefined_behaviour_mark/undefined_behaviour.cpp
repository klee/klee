#include <iostream>
#include <thread>
#include "gtest/gtest.h"

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
