/*
    @basic dynamic task scheduling, greedy strategy based on node's level.

    @detail (1) using topological sorting to convert task dependency graph (DAG) to a tree-struction.
            (2) scheduling strategy:
                step 1. add all leaf nodes to Q. (Q is a priority_queue, sorting by node's level)
                step 2. if a node has been finished, check the dependency of its parent node. 
                    if its parent node has no dependency, add this parent node to Q.

    @author lszero
    @version    3.0
    @created   2017/03/20
    @last_updated   2017/03/21
*/

#include <iostream>
#include <stdio.h>
#include <thread>
#include <future>
#include <unordered_map>
#include <vector>
#include <queue>
#include <stack>
#include <queue>
#include <sstream>
#include <sys/time.h>
#include <mutex>
#include "ThreadPool/ThreadPool.h"
#include "Task.h"
#include "NodeInfo.h"
using namespace std;

#include <boost/program_options.hpp>
namespace po = boost::program_options;


ThreadPool *pool = NULL;
mutex mtx;
bool finish = false;    // whether all tasks have been finished
unordered_map<NodeInfo*, future<string>> task_results;


vector<NodeInfo*> nodes;
unordered_map<NodeInfo*, vector<NodeInfo*>> tree;
unordered_map<NodeInfo*, NodeInfo*> parents;
// push node with no outcoming edges into queue. (sorting by node's level)
priority_queue<NodeInfo*, vector<NodeInfo*>, cmp> Q;


void initDAG(vector<Task*> &tasks, unordered_map<Task*, vector<Task*>> &DAG){
    // create a DAG
    int num_tasks = 8;
    
    tasks.push_back(new Task(-1)); // dummy
    for(int i = 1; i <= num_tasks; i++){
        tasks.push_back(new Task(i));
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

// data processing: merge all children's info
string process_data(NodeInfo *node, unordered_map<NodeInfo*, vector<NodeInfo*>> &tree){
    this_thread::sleep_for(chrono::seconds(1));
    string new_data;
    int n = tree[node].size();
    for(int i = 0; i < n; i++){
        new_data += tree[node][i]->p_task->data + " ";
    }
    new_data += "x" + to_string(node->p_task->id);
    node->p_task->data = new_data;
    return new_data;
}

// declear thread function
string thread_fun(NodeInfo *node, unordered_map<NodeInfo*, vector<NodeInfo*>> &tree);

// update tree's info
void update_tree(NodeInfo *node, unordered_map<NodeInfo*, vector<NodeInfo*>> &tree){
    lock_guard<mutex> lk(mtx);  // lock
    if(node->p_task->id == 1){  // all tasks have done
        finish = true;
        return;
    }

    NodeInfo *prt = parents[node];
    prt->n_dependency--;
    if(prt->n_dependency == 0){
        Q.push(prt);
        NodeInfo *node = Q.top();
        Q.pop();
        // task_results.emplace_back(
        //     pool->add_job(thread_fun, node, tree)
        // );
        task_results[node] = pool->add_job(thread_fun, node, tree);
        cout << "add task_" << node->p_task->id << endl;
    }
}

// thread function
string thread_fun(NodeInfo *node, unordered_map<NodeInfo*, vector<NodeInfo*>> &tree){
    string rst = process_data(node, tree);
    update_tree(node, tree);
    return rst;
}

// topological sorting: convert DAG to tree-struction
unordered_map<NodeInfo*, vector<NodeInfo*>> topo_sort(vector<Task*> &tasks, unordered_map<Task*, vector<Task*>> &DAG){
    // the number of incoming edges for each node
    unordered_map<Task*, int> in;

    int num_tasks = tasks.size()-1;
    for(int i = 1; i <= num_tasks; i++){
        in[tasks[i]] = 0;
    }
    
    for(auto it = DAG.begin(); it != DAG.end(); it++){
        for(int i = 0; i < it->second.size(); i++){
            in[it->second[i]]++;
        }
    }

    queue<Task*> que;
    for(auto it = in.begin(); it != in.end(); it++){
        if(it->second == 0){
            que.push(it->first);
        }
    }

    // mapping: Task -> NodeInfo
    unordered_map<Task*, NodeInfo*> map;
    for(int i = 1; i <= num_tasks; i++){
        nodes.push_back(new NodeInfo(tasks[i]));
        map[tasks[i]] = nodes[nodes.size()-1];
    }

    unordered_map<NodeInfo*, vector<NodeInfo*>> tree; 
    int level = 0;
    while(!que.empty()){
        int n = que.size();
        while(n--){
            Task* now = que.front();
            que.pop();

            map[now]->level = level;
            
            if(DAG[now].size() == 0){ 
                Q.push(map[now]);
            }

            int cnt = 0;
            for(int i = 0; i < DAG[now].size(); i++){
                in[DAG[now][i]]--;
                if(in[DAG[now][i]] == 0){
                    que.push(DAG[now][i]);

                    tree[map[now]].push_back(map[ DAG[now][i] ]);
                    parents[ map[ DAG[now][i] ] ] = map[now];
                    cnt++;
                }
            }
            map[now]->n_dependency = cnt;
        }
        level++;
    }

    // print nodes' info
    cout << "all nodes' info:" << endl;
    for(int i = 0; i < nodes.size(); i++){
        cout << "node_" << nodes[i]->p_task->id << ": level="<< nodes[i]->level << " d="<<nodes[i]->n_dependency<< endl;
    }
    cout << endl;

    // print tree-struction
    cout << "tree-struction:" << endl;
    for(auto it = tree.begin(); it != tree.end(); it++){
        for(int j = 0; j < it->second.size(); j++){
            cout << it->first->p_task->id << " " << it->second[j]->p_task->id << endl;
        }
    }
    cout << endl;

    return tree;
}

// dynamic task scheduling
void scheduling(unordered_map<NodeInfo*, vector<NodeInfo*>> &tree, ThreadPool *pool){
    while(!Q.empty()){
        NodeInfo *node = Q.top();
        Q.pop();
        // task_results.emplace_back(
        //     pool->add_job(thread_fun, node, tree)
        // );
        task_results[node] = pool->add_job(thread_fun, node, tree);
        cout << "add task_" << node->p_task->id << endl;
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
    pool = new ThreadPool(num_threads);

    // init task dependency graph (DAG)
    vector<Task*> tasks;
    unordered_map<Task*, vector<Task*>> DAG;
    initDAG(tasks, DAG);

    struct timeval start, end;
    double topolsort_dura, processtasks_dura;

    // get topological order (a tree struction)
    gettimeofday(&start, 0);
    tree = topo_sort(tasks, DAG);
    gettimeofday(&end, 0);
    topolsort_dura = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000.0;

    // task scheduling
    gettimeofday(&start, 0);
    scheduling(tree, pool);
    while(!finish){}    // waiting for all tasks to be finished
    gettimeofday(&end, 0);
    processtasks_dura = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000.0;

    // print all tasks result
    cout << "\nresult:" << endl;
    for(int i = 0; i < nodes.size(); i++){
        cout << "task_" << (i+1) << ": " << task_results[nodes[i]].get() << endl;
    }

    // print execution time
    printf("\ntopological sorting execution time: %.2f s\n", topolsort_dura);
    printf("process all tasks execution time: %.2f s\n", processtasks_dura);

    delete pool;

    return 0;
}
