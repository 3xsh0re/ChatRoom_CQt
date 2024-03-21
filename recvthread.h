#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include<QThread>

class RecvThread:public QThread
{
public:
    RecvThread();
    //重写run函数
    // void run();

};

#endif // RECVTHREAD_H
