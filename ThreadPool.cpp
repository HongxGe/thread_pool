#include "ThreadPool.h"

#include <thread>
#include <iostream>

const int TASK_MAX_THREADHOLD = 1024;

ThreadPool::ThreadPool()
        : initThreadSize_(4)
        , taskSize_(0)
        , taskQueueThreadHold_(TASK_MAX_THREADHOLD)
        , poolMode(ThreadPoolMode::MODE_FIXED){}

ThreadPool::~ThreadPool() {

}

// 设置工作模式
void ThreadPool::setMode(ThreadPoolMode mode) {
    poolMode = mode;
}

// 设置Task任务队列上限
void ThreadPool::setTaskQueueMaxThreadHold(int threadHold) {
    taskQueueThreadHold_ = threadHold;
}

// 给线程池提交任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp) {

}

//开启线程池
void ThreadPool::start(int initThreadSize) {
    // 记录初始线程数量
    initThreadSize_ = initThreadSize;

    // 创建线程对象的时候，把线程函数给到thread线程对象
    for (int i = 0; i < initThreadSize_; ++i) {
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::ThreadFunc, this)));
    }

    // 启动所有线程
    for (int i = 0; i < initThreadSize_; ++i) {
        threads_[i]->start();
    }

}

// 定义线程函数
void ThreadPool::ThreadFunc() {
    std::cout << "begin threadFunc tid : " <<std::this_thread::get_id() << std::endl;

    std::cout << "end threadFunc tid : " <<std::this_thread::get_id() << std::endl;
}


/******************************************************************
 * 线程方法实现
******************************************************************/
// 线程构造
Thread::Thread(ThreadFunc func) : func_(func) {

}
// 线程析构
Thread::~Thread() {

}
void Thread::start() {
    // 执行线程函数
    //创建一个线程来执行一个线程函数
    std::thread t(func_);  // 对于C++11来说：有线程对象t，和线程函数func_
    // 线程对象t在start结束时，线程对象t的作用域周期到了，t和func_是有绑定关系的,两者一起挂
    t.detach();  // 设置成分离线程，线程对象t结束时，func_还在执行  -->linux thread库·pthread_detach pthread_t设置成分离线程
}