#include "ThreadPool.h"

#include <thread>
#include <iostream>

const int TASK_MAX_THREADHOLD = 4;

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

// 给线程池提交任务  用户向线程池里面的任务队列提交任务 生产者
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {
    // 获取锁
    std::unique_lock<std::mutex> lock(taskQueueMtx_);

    // 线程的通信： 等待任务队列有空余
    /*这里wait方法有两种重载实现*/
    /*1*/
    // if (tasksQueue_.size() == TASK_MAX_THREADHOLD) {
    //     nofull_.wait(lock);
    // }
        //条件变量的wait重载
        //para1：用到的unique_lock锁
        //para2：等待该条件成立
    /*2*/
    //nofull_.wait(lock, [&]()-> bool {return tasksQueue_.size() < TASK_MAX_THREADHOLD;});
    //用户提交任务，最长不能阻塞超过1s，否则判断任务提交失败，返回
    if(!nofull_.wait_for(lock, std::chrono::seconds(1), 
        [&]()->bool{return tasksQueue_.size() < (size_t)TASK_MAX_THREADHOLD;}))
    {
        // 表示nofull_等待1s，条件依然没有满足
        std::cerr << "task queue is full, submit task fail." << std::endl;
        return Result(sp, false);
    }

    std::cout << "tid:" << std::this_thread::get_id()
            << "将任务放到任务队列中!" << std::endl;
    // 如果为空，把任务放到任务队列中
    tasksQueue_.emplace(sp);
    taskSize_++;
    // 提示消费者任务队列不为空 在noempty_提示,通知消费者，可以赶紧分配任务
    noempty_.notify_all();

    // 返回任务的返回值对象
    return Result(sp);


}

//开启线程池
void ThreadPool::start(int initThreadSize) {
    // 记录初始线程数量
    initThreadSize_ = initThreadSize;

    // 创建线程对象的时候，把线程函数给到thread线程对象
    for (int i = 0; i < initThreadSize_; ++i) {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::ThreadFunc, this));
        threads_.emplace_back(std::move(ptr));
    }

    // 启动所有线程
    for (int i = 0; i < initThreadSize_; ++i) {
        threads_[i]->start();
    }

}

// 定义线程函数  线程池里面所有的线程从任务队列里面消费任务  消费者
void ThreadPool::ThreadFunc() {
    // std::cout << "begin threadFunc tid : " <<std::this_thread::get_id() << std::endl;

    // std::cout << "end threadFunc tid : " <<std::this_thread::get_id() << std::endl;
    for (;;) {
        std::shared_ptr<Task> task;
        {
            // 获取锁
            std::unique_lock<std::mutex> lock(taskQueueMtx_);
            // 等待noempty条件
                // this捕获 表明要访问当前类
                // &引用捕获 表示捕获所有外部变量
                // &当上下文中有其它局部变量或者对象时，也都会被捕获
                // 下面两种实现都是可以的
            std::cout << "tid:" << std::this_thread::get_id()
            << "try to get the task!" << std::endl;
            noempty_.wait(lock, [this]( ) -> bool {return !tasksQueue_.empty();});
            //noempty_.wait(lock, [&]( ) -> bool {return tasksQueue_.size() > 0;});

            std::cout << "tid:" << std::this_thread::get_id()
            << "got the task successfully!" << std::endl;
            // 从任务队列中取一个任务
            task = tasksQueue_.front();
            tasksQueue_.pop();
            taskSize_--;

            // 如果依然由任务，继续通知其它线程（消费者）执行任务
            if (tasksQueue_.size() > 0) {
                noempty_.notify_all();
            }
            // 取出任务，通知生产者任务队列不满
            nofull_.notify_all();
        }
        //应该把锁释放掉,那么可以把上面用局部作用域框起来
        // 或者这里可以显性地解锁lock.unlock();

        // 由当前线程执行该任务
        if (NULL != task) {
            task->run();
            //执行完一个任务,怎么去把这个Task任务delete？
        }
    }
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
/******************************************************************
 * Task方法实现
******************************************************************/
// 为什么这里需要给run再封装一次？
/**
 * 因为
 */
void Task::exec() {
    result_->setVal(run());  
    //run();  // 这里发生多态调用
}

/******************************************************************
 * Result方法实现
******************************************************************/
Result::Result(std::shared_ptr<Task> task, bool isVaild = true)
    : task_(task)
    , isVaild_(isVaild) {
    task_->setResult(this);
}
void Result::setVal(Any any) {  // 任务执行完的时候调用
    // 存储Task的返回值any对象
    any_ = std::move(any);
    // 已经获取任务的返回值，增加信号量的资源，线程通信
    sem_.post();
}
 
Any Result::get() {  // 用户调用的
    if (!isVaild_) return "";
    sem_.wait();  // Task任务如果没有执行完，这里会阻塞用户的线程。
    return std::move(any_);
}