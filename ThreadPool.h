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

// Any类型：可以接受任意数据的类型
class Any {
public:
    Any() = default;
    ~Any() = default;
    // 左值的引用拷贝构造和赋值???
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;
    // 右值的引用拷贝构造和赋值???
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    template <typename T>  //T:int Driver<int>
    Any(T data) : base_(std::make_unique<Driver>(data)) {}

    // 把Any对象存储的数据提取出来
    template <typename T>
    T cast_() {
        // 我们怎么从base_找到它所指向的Driver对象，从它里面取出data成员变量
        // 基类指针 -> 派生类指针
        Driver<T> *pd = dynamic_cast<Driver<T>>(base_.get());
        if (pd == nullptr) {
            throw "type is unmatch!";
        }
        return pd->data_;
    };

private:
    // 基类类型
    class Base {
    public:
        virtual ~Base() = default;
    };

    template <typename T>
    class Driver : public Base {
    public:
        Driver(T data) : data_(data) {}
        T data_;
    };

    std::unique_ptr<Base> base_;
};

// 实现一个信号量类
class Semaphore {
public:
    Semaphore(int limit = 0) : limit_(limit) { limit_ = 0;}
    ~Semaphore() = default;

    // 获取等待一个信号量资源
    void wait () {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待信号量不为空
        cond_.wait(lock, [this]()-> bool {return limit_ > 0;});
        limit_--;
    }
    // 增加一个信号量资源
    void post () {
        std::unique_lock<std::mutex> lock(mutex_);
        limit_++;
        cond_.notify_all();
    }
private:
    std::mutex mutex_;
    std::condition_variable cond_;
    int limit_;
};

// Task类型的前置声明
class Task;

// 实现接受提交到线程池的Task任务执行结束之后的返回值类型Result
class Result {
public:
    Result() = default;
    ~Result() = default;

    Result(std::shared_ptr<Task> task, bool isVaild = true);
    Result(const Result& value) = default;
    Result& operator=(const Result& value)= default;
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;

    // setVal方法，获取任务执行完的返回值
    void setVal(Any any);

    // get方法，用户调用个体方法获取任务的返回值
    Any get();


private:
    Any any_;  // 存储任务的返回数据
    Semaphore sem_;  // 线程通信信号量
    std::shared_ptr<Task> task_; // 指向对应获取返回值的任务对象
    std::atomic_bool isVaild_;  // 返回值是否有效
};

// 任务抽象基类
class Task {
public:
    // 用户可以自定义容易任务类型，从Task继承，重写run方法，实现自定义的任务处理
    virtual Any run() = 0;
    void exec();
    void setResult(Result* result);
private:
  Result* result_;
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

/*
example
ThreadPool pool;
pool.start(4);

class MyTask : public task{
public: 
    void run() { // 线程代码}；
}
pool.submitTask(std::make_shared<MyTask>());
*/
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
    Result submitTask(std::shared_ptr<Task> sp);

    //开启线程池
    void start(int initThreadSize = 4);

    //禁止线程池拷贝构造
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

// 定义线程函数
private:
    void ThreadFunc();

private:
    std::vector<std::unique_ptr<Thread>> threads_;  // 线程列表
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