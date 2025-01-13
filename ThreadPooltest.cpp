#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib> // for atoi

#include "ThreadPool.h"

class Mytask : public Task {
public:
    Mytask(size_t begin, size_t end):begin_(begin), end_(end) {}

    // 怎么去设计run方法地返回值,可以表示任意的类型
    // 首先模板函数和虚函数是不能共存的，所以这里不能使用模板
    // java,python中有一个object类，是所有类型的基类
    // c++17 Any类
    Any run() {
        std::cout << "tid:" << std::this_thread::get_id()
            << "begin!" << std::endl;
        auto sum = 0;
        for (auto it = begin_; begin_ <= end_; ++it) {
            sum += it;
        }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "tid:" << std::this_thread::get_id()
            << "end!" << std::endl;
        Any sum_(sum);
        return sum_;
    }
private:
    size_t begin_;
    size_t end_;
};

int main(int argc, char* argv[]) {
    if (0 == argc) return 0;
    ThreadPool pool;
    pool.setTaskQueueMaxThreadHold(atoi(argv[2]));
    pool.start(atoi(argv[1]));

    // 如何设计Result机制
    Result res = pool.submitTask(std::make_shared<Mytask>(1,100));
    pool.submitTask(std::make_shared<Mytask>(101,300));
    pool.submitTask(std::make_shared<Mytask>(301,1000));

    getchar();

    //std::this_thread::sleep_for(std::chrono::seconds(5));

}