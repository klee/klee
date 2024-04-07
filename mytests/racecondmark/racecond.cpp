#include <iostream>
#include <thread>
#include <mutex>
#include "gtest/gtest.h"

int sharedVariable = 0;
std::mutex mutex; // Мьютекс для синхронизации доступа к общей переменной

void updateVariable(int value) {
std::lock_guard<std::mutex> lock(mutex);
sharedVariable = value;
}

void thread1() {
updateVariable(123);
}

void thread2() {
updateVariable(456);
}

int main() {
setlocale(LC_ALL, "Russian");
std::thread t1(thread1);
std::thread t2(thread2);

t1.join();
t2.join();

std::cout << "Значение переменной: " << sharedVariable << std::endl;

return 0;
}
