#pragma once
#include<string>
#include<random>
#include<iostream>
#include<mutex>
#include<utility>


class utils{
public:
    template<typename U>
    static void SafePrint(U&& st){
        std::unique_lock lock{umutex};
        std::cout << std::forward<U>(st) << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    static void RandomSleep(int minMs, int maxMs, double scale){
        std::uniform_int_distribution<> dist(minMs, maxMs);
        auto ms = static_cast<int>(dist(eng) * scale);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

private:
    static inline std::mutex umutex{};
    static inline std::mt19937 eng{std::random_device{}()};

};