#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <vector>
using namespace std;

class ThreadPool{
public:
    ThreadPool(int num_threads){
        n_threads = num_threads;
        stop = false;

        while(num_threads--){
            threads.emplace_back(thread(   // emplace_back avoids the extra copy or move operation required when using push_back.
                [this](){
                    while(true){    // these threads always run, waiting for job to do.

                        function<void()> task;

                        {
                            unique_lock<mutex> lk(this->mtx);
                            this->cv.wait(lk, [this](){ return this->stop || !this->jobQueue.empty(); });
                            if(this->stop && this->jobQueue.empty()){
                                return;
                            }
                            task = std::move(this->jobQueue.front());
                            jobQueue.pop();
                        }   // unlock

                        task();
                    }
                }
            ));
        }
    }

    // add job to jobQueue
    template<class Function, class... Args>
    auto add_job(Function&& f, Args&&... args)
        -> future<typename std::result_of<Function(Args...)>::type> 
    {
        using return_type = typename std::result_of<Function(Args...)>::type;
        auto task = std::make_shared< packaged_task<return_type()> >(
            std::bind(std::forward<Function>(f), std::forward<Args>(args)...)
        );
        future<return_type> rst = task->get_future();

        {
            lock_guard<mutex> lk(mtx);
            jobQueue.emplace([task](){ (*task)(); });
        }
        
        cv.notify_one();
        return rst;
    }

    ~ThreadPool(){
        {
            lock_guard<mutex> lk(mtx);
            stop = true;
        }
        cv.notify_all();
        for(int i = 0; i < threads.size(); i++){
            threads[i].join();
        }
    }

private:
    mutex mtx;
    condition_variable cv;
    int n_threads;
    vector<thread> threads;
    queue<function<void()>> jobQueue;
    bool stop;
};

#endif
