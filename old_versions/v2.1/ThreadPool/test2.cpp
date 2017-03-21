#include <thread>
#include <future>
#include <vector>
#include <iostream>
#include "ThreadPool.h"
using namespace std;

class Task{
public:
    Task(int val) : x(val){}

    int task_fun(int y){
        this_thread::sleep_for(chrono::seconds(1));
        return x * y;
    }

private:
    int x;
};

int main(){
    int num_threads = thread::hardware_concurrency();
    ThreadPool pool(num_threads);
    
    vector<future<int>> rsts;
    int num_jobs = 40;

    vector<Task> tasks;
    for(int i = 0; i < num_jobs; i++){
        tasks.emplace_back(Task(i));
    }

    for(int i = 0; i < num_jobs; i++){
        rsts.emplace_back(
            pool.add_job(&Task::task_fun, &tasks[i], i) // non-static member function
        );
    }

    // get results
    for(int i = 0; i < rsts.size(); i++){
        cout << rsts[i].get() << endl;
    }
    return 0;
}
