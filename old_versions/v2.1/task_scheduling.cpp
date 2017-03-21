/*
    @basic level-based static task scheduling.

    @detail (1) using topological sorting to convert task dependency graph (DAG) to a tree-struction.
            (2) for each level in tree-struction, adding all tasks to thread pool.

    @author lszero
    @version    2.1
    @created   2017/03/14
    @last_updated   2017/03/15
*/

#include <iostream>
#include <stdio.h>
#include <thread>
#include <future>
#include <unordered_map>
#include <vector>
#include <queue>
#include <stack>
#include <sstream>
#include "ThreadPool/ThreadPool.h"
#include <sys/time.h>
using namespace std;

#include <boost/program_options.hpp>
namespace po = boost::program_options;


// task node
class TaskNode{
public:
    int id;
    string data;
    TaskNode(int val) : id(val) {}
};

void initDAG(vector<TaskNode*> &tasks, unordered_map<TaskNode*, vector<TaskNode*>> &DAG){
    // create a DAG
    int num_tasks = 8;
    
    tasks.push_back(new TaskNode(-1)); // dummy
    for(int i = 1; i <= num_tasks; i++){
        tasks.push_back(new TaskNode(i));
    }
    // DAG[tasks[0]].push_back(tasks[1]);

    DAG[tasks[1]].push_back(tasks[2]);
    DAG[tasks[1]].push_back(tasks[3]);
    DAG[tasks[1]].push_back(tasks[4]);
    DAG[tasks[2]].push_back(tasks[5]);
    DAG[tasks[2]].push_back(tasks[6]);
    DAG[tasks[3]].push_back(tasks[4]);
    DAG[tasks[3]].push_back(tasks[7]);
    DAG[tasks[4]].push_back(tasks[7]);
    DAG[tasks[4]].push_back(tasks[8]);
}

// task function: data processing (merge all children's info)
string task_fun(TaskNode *node, unordered_map<TaskNode*, vector<TaskNode*>> &DAG){
    this_thread::sleep_for(chrono::seconds(1));
    string new_data;
    for(int i = 0; i < DAG[node].size(); i++){
        new_data += DAG[node][i]->data + " ";
    }
    new_data += "x" + to_string(node->id);
    node->data = new_data;
    return node->data;
}

// topological sorting: convert DAG to tree-struction
vector<vector<TaskNode*>> topo_sort(vector<TaskNode*> &tasks, unordered_map<TaskNode*, vector<TaskNode*>> &DAG){
    // the number of incoming edges for each node
    unordered_map<TaskNode*, int> in;

    int num_tasks = tasks.size()-1;
    for(int i = 1; i <= num_tasks; i++){
        in[tasks[i]] = 0;
    }
    
    for(auto it = DAG.begin(); it != DAG.end(); it++){
        for(int i = 0; i < it->second.size(); i++){
            in[it->second[i]]++;
        }
    }

    queue<TaskNode*> Q;
    for(auto it = in.begin(); it != in.end(); it++){
        if(it->second == 0){
            Q.push(it->first);
        }
    }

    vector<vector<TaskNode*>> topolOrder;   // topolOrder[i]: tasks in i-th level
    int level = 0;
    while(!Q.empty()){
        int n = Q.size();
        topolOrder.push_back(vector<TaskNode*>());

        while(n--){
            TaskNode* now = Q.front();
            Q.pop();
            topolOrder[level].push_back(now);
            for(int i = 0; i < DAG[now].size(); i++){
                in[DAG[now][i]]--;
                if(in[DAG[now][i]] == 0){
                    Q.push(DAG[now][i]);
                }
            }
        }
        level++;
    }

    // print topological order
    cout << "Topological order:" << endl;
    for(int i = 0; i < topolOrder.size(); i++){
        cout << "in level_" << i+1 << ":";
        for(int j = 0; j < topolOrder[i].size(); j++){
            cout << " " << topolOrder[i][j]->id;
        }
        cout << endl;
    }

    return topolOrder;
}

// scheduling strategy: for each level in tree-struction (bottom-up), adding all tasks to thread pool.
void scheduling(vector<vector<TaskNode*>> &topolOrder, unordered_map<TaskNode*, vector<TaskNode*>> &DAG, ThreadPool &pool){
    cout << endl << "results:" << endl;
    for(int i = topolOrder.size()-1; i >= 0; i--){
        vector<future<string>> results;

        for(int j = 0; j < topolOrder[i].size(); j++){
            results.emplace_back(
                pool.add_job(task_fun, topolOrder[i][j], DAG) 
            );
        }

        for(int j = 0; j < results.size(); j++){
            cout << "task_" << "" << topolOrder[i][j]->id << " result: " << results[j].get() << endl;
        } 
    }
}


int main(int argc, char** argv){
    // parse program options
    po::options_description opt_desc("Options");
    opt_desc.add_options()
        ("help", "produce help message")
        ("nthreads", po::value<int>(), "set size of thread pool")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opt_desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << opt_desc << "\n";
        return 0;
    }

    // set number of threads
    int num_threads = thread::hardware_concurrency();   // default size of thread pool
    if (vm.count("nthreads")){
        num_threads = vm["nthreads"].as<int>();
    }
    cout << endl << "nthreads: " << num_threads << endl << endl;

    // create a thread pool
    ThreadPool pool(num_threads);

    // init task dependency graph (DAG)
    vector<TaskNode*> tasks;
    unordered_map<TaskNode*, vector<TaskNode*>> DAG;
    initDAG(tasks, DAG);

    struct timeval start, end;
    double topolsort_dura, processtasks_dura;

    // get topological order (a tree struction)
    gettimeofday(&start, 0);
    vector<vector<TaskNode*>> topolOrder = topo_sort(tasks, DAG);
    gettimeofday(&end, 0);
    topolsort_dura = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000.0;

    // task scheduling
    gettimeofday(&start, 0);
    scheduling(topolOrder, DAG, pool);
    gettimeofday(&end, 0);
    processtasks_dura = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000.0;

    // print execution time
    printf("\ntopological sorting execution time: %.2f s\n", topolsort_dura);
    printf("process all tasks execution time: %.2f s\n", processtasks_dura);
    return 0;
} 
