#ifndef _TASK_H_
#define _TASK_H_

using namespace std;

class Task{
public:
    Task(int val) : id(val) {}

public:
    int id;
    string data;
};

#endif
