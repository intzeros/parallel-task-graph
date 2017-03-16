# parallel-task-graph
Task scheduling based on thread pool. Getting parallel tasks for a given dependency graph (or task graph), which is a DAG, via topological sorting. Then put these tasks into thread pool.

An input dependency graph looks like this: (in main.cpp)

<img src="https://github.com/lszero/parallel-task-graph/blob/master/DAG.png" width = "300" height = "200" alt="DAG" align=center />

# usage
```shell
g++ -std=c++11 -lboost_program_options main.cpp -o test
./test --help
./test --nthread 4
```
