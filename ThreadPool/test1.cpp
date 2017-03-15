#include <thread>
#include <future>
#include <vector>
#include <iostream>
#include "ThreadPool.h"
using namespace std;

int task_fun(int i){
    this_thread::sleep_for(chrono::seconds(1));
    return i*i;
}

int main(){
    int num_threads = thread::hardware_concurrency();
    ThreadPool pool(num_threads);
    
    vector<future<int>> rsts;
    int num_jobs = 40;

    for(int i = 0; i < num_jobs; i++){
        rsts.emplace_back(
            pool.add_job(task_fun, i)   // function pointer
        );
    }

    // get results
    for(int i = 0; i < rsts.size(); i++){
        cout << rsts[i].get() << endl;
    }
    return 0;
}