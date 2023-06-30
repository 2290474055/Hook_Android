//
// Created by wb.mzy on 2023/6/29.
//

#ifndef SECCOMPTEXT_TASK_H
#define SECCOMPTEXT_TASK_H
#include <jni.h>
#include <string>
#include <stdio.h>
#include <sys/prctl.h>
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <stdlib.h>
#include <unistd.h>
#include <android/log.h>
#include <pthread.h>
#include <list>
#include <mutex>
#endif //SECCOMPTEXT_TASK_H

//定义hook svc的宏
#define NT_exit 0x5e
#define NT_kill 0x81
#define NT_openat 0x38

#define TAG "zeyu"

//该互斥锁是为了防止在处理系统调用的线程中list获取Task与在list去除Task出现问题，因为写的死循环，后期会考虑改进一下
std::mutex mi;

class Task{
    public:
        bool call;//是否在线程池中调用过
        int syscall;//系统调用号
        int parametersNum;//参数数量
        long ret;//返回值
        void* parameters[20];//所有参数
        Task(){
            call = 0;
            syscall = 0;
            parametersNum = 0;
            ret = 0;
            for(int i=0;i<20;i++){
                parameters[i]  = 0;
            }
        }
        Task(int m_syscall,int m_parametersNum,long** m_parameters){
            call = false;
            syscall = m_syscall;
            parametersNum = m_parametersNum;
            ret = 0;
            for(int i=0;i<parametersNum;i++){
                parameters[i]  = (void*)(m_parameters[i]);
            }
        }
};

//处理线程，主要用来不断检查是否有Task，如果有并且没有调用过，则调用一次。否则则一致检查
void* WorkThread(void* arg){
    std::list<Task*>* TaskList  = (std::list<Task*>*)arg;
    while(true){
        mi.lock();
        if (!TaskList->empty()) {
            Task* work = TaskList->back();
            if(work->call == false){
                switch (work->parametersNum) {
                    case 0:
                        work->ret = syscall(work->syscall);
                        break;
                    case 1:
                        work->ret = syscall(work->syscall,work->parameters[0]);
                        break;
                    case 2:
                        work->ret = syscall(work->syscall,work->parameters[0],work->parameters[1]);
                        break;
                    case 3:
                        work->ret = syscall(work->syscall,work->parameters[0],work->parameters[1],work->parameters[2]);
                        break;
                    case 4:
                        work->ret = syscall(work->syscall,work->parameters[0],work->parameters[1],work->parameters[2],work->parameters[3]);
                        break;
                    case 5:
                        work->ret = syscall(work->syscall,work->parameters[0],work->parameters[1],work->parameters[2],work->parameters[3],work->parameters[4]);
                        break;
                    case 6:
                        work->ret = syscall(work->syscall,work->parameters[0],work->parameters[1],work->parameters[2],work->parameters[3],work->parameters[4],work->parameters[5]);
                        break;
                    case 7:
                        work->ret = syscall(work->syscall,work->parameters[0],work->parameters[1],work->parameters[2],work->parameters[3],work->parameters[4],work->parameters[5],work->parameters[6]);
                        break;
                    case 8:
                        work->ret = syscall(work->syscall,work->parameters[0],work->parameters[1],work->parameters[2],work->parameters[3],work->parameters[4],work->parameters[5],work->parameters[6],work->parameters[7]);
                        break;
                }
                work->call = true;
            }
        }
        mi.unlock();
    }
}

//该类主要用于创建系统调用线程，以及相关处理
class Pthread_pool{
    public:
        std::list<Task*> TaskList;
        pthread_t id;
        Pthread_pool(){
            id = {0};
            int ret = pthread_create(&id, NULL, &WorkThread,&TaskList);
            if(ret == 0){

            }
        }
        //将Task塞入list
        bool setTask(Task* work){
            TaskList.push_back(work);
            return true;
        }
        //等待线程调用结束
        void waitCall(){
            Task* newWork  = TaskList.back();
            while(true){
                if(newWork->call==true){
                    break;
                }
            }
            return;
        }
        //将Task去除list中
        void cleanTask(){
            //__android_log_print(ANDROID_LOG_INFO,TAG ,"openat:7");
            mi.lock();
            TaskList.pop_back();
            mi.unlock();
        }
        //封装一层
        void sendTask_waitCall_cleanTask(Task* work){
            setTask(work);
            waitCall();
            cleanTask();
        }

        ~Pthread_pool(){
            pthread_kill(id, SIGUSR1);
        }
};

