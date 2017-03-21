#ifndef _NODE_INFO_H_
#define _NODE_INFO_H_

#include "Task.h"

class NodeInfo{
public:
    NodeInfo(){
        p_task = NULL;
        n_dependency = 0;
        level = 0;
    }

    NodeInfo(Task* task){
        p_task = task;
        n_dependency = 0;
        level = 0;
    }
    
    NodeInfo(Task* task, int _n_dependency, int _level){
        p_task = task;
        n_dependency = _n_dependency;
        level = _level;
    }

    // bool operator<(const NodeInfo &node) const {
    //     return level < node.level;
    // }

    // bool operator>(const NodeInfo &node) const {
    //     return level > node.level;
    // }

public:
    Task *p_task;
    int n_dependency;
    int level;
};


struct cmp{
    bool operator()(const NodeInfo *n1, const NodeInfo *n2){
        return n1->level < n2->level;
    }
};

#endif
