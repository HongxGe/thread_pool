#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>


// 任务抽象基类
class Task {
public:
    // 用户可以自定义容易任务类型，从Task继承，重写run方法，实现自定义的任务处理
    virtual void run() = 0;
};
// 线程池模式
// ??class，杜绝枚举类型不同，枚举项相同的情况
enum class ThreadPoolMode {
    MODE_FIXED,    // 固定线程数量
    MODE_CACHED,   // 线程数量可动态增长
};
// 线程类型
class Thread {
public:
    // 线程函数对象类型
    using ThreadFunc = std::function<void()>;
    // 线程构造
    Thread(ThreadFunc func);
    // 线程析构
    ~Thread();

    // 启动线程
    void start();
private:
    ThreadFunc func_;
    
};
// 线程池类型
class ThreadPool {
public:
    // 线程池构造
    ThreadPool();
    // 线程池析构
    ~ThreadPool();

    // 设置工作模式
    void setMode(ThreadPoolMode mode);

    // 设置Task任务队列上限
    void setTaskQueueMaxThreadHold(int threadHold);

    // 给线程池提交任务
    void submitTask(std::shared_ptr<Task> sp);

    //开启线程池
    void start(int initThreadSize = 4);

    //禁止线程池拷贝构造
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

// 定义线程函数
private:
    void ThreadFunc();

private:
    std::vector<Thread*> threads_;  // 线程列表
    size_t initThreadSize_;  // 初始线程数量

    // 基类裸指针 -? 使用智能指针能保证类的生命周期嘛
    // 基类智能指针 保证类的生命周期，而且能自动释放资源
    std::queue<std::shared_ptr<Task>> tasksQueue_;  // 任务队列
    std::atomic_int taskSize_;  // 任务的数量 需要考虑线程安全，但不需要用到mutex+condition_variable那么重的同步机制
    int taskQueueThreadHold_; // 任务队列数量阈值
    std::mutex taskQueueMtx_;  // 保证任务队列的线程安全
    std::condition_variable noempty_;  // 任务队列不为空
    std::condition_variable nofull_;  // 任务队列不满
    ThreadPoolMode poolMode;  // 当前线程池工作模式
};

#endif