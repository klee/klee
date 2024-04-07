#include <iostream>
#include <thread>
#include <mutex>
#include "gtest/gtest.h"

int val1=0;
TEST(TestTopic, TrivialEquality)
{
    EXPECT_EQ(val1, 0);
}

std::mutex mutex1;
std::mutex mutex2;

void process1() {
std::lock_guard<std::mutex> lock1(mutex1);
std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Имитация обработки
std::lock_guard<std::mutex> lock2(mutex2);

// Критическая секция процесса 1
std::cout << "Процесс 1 выполняет работу." << std::endl;
}

void process2() {
std::lock_guard<std::mutex> lock2(mutex2);
std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Имитация обработки
std::lock_guard<std::mutex> lock1(mutex1);

// Критическая секция процесса 2
std::cout << "Процесс 2 выполняет работу." << std::endl;
}

int main() {
setlocale(LC_ALL, "Russian");
std::thread t1(process1);
std::thread t2(process2);

t1.join();
t2.join();

return 0;
}
