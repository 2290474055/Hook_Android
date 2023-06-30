#include "Task.h"
#include <errno.h>


//用来与我简陋的线程池交互的类
Pthread_pool* Stak = nullptr;
//互斥锁,此锁用来确保每次信号回调只会有一个正在执行，主要是为了防止同时触发信号。导致处理调用的线程出现错误
std::mutex mu;

void sigsys_handler(int signum, siginfo_t *info, void *ucontext) {
    if (info->si_signo == SIGSYS) {
        //获取锁，保证只有一次正在处理，主要是因为只有一个线程，怕乱处理返回值错了
        mu.lock();
        Task mework;
        struct ucontext *context = (struct ucontext *)ucontext;
        //此处进行分类处理。参数储存在寄存器中，拿到寄存器值即可。
        if(info->si_syscall == NT_kill){
            __android_log_print(ANDROID_LOG_INFO,TAG ,"kill sigsys_handler");
            //此处条件编译是因为得到保存寄存器信息的结构体类型不一致导致的，下面有一个arm的例子。
            #if defined(__aarch64__)
                //获取寄存器信息
                sigcontext mcontext = context->uc_mcontext;
                //将相关数据填入自己定义的要发送给线程的结构体，对于kill函数信号为info->si_syscall,函数参数有2个,参数1与参数2分别存放在mcontext.regs[0]，mcontext.regs[1]
                mework.syscall = info->si_syscall;
                mework.parametersNum = 2;
                mework.parameters[0] = (void*)mcontext.regs[0];
                mework.parameters[1] = (void*)mcontext.regs[1];

                __android_log_print(ANDROID_LOG_INFO,TAG ,"kill arg:%x   %x   mepid:%x",mework.parameters[0],mework.parameters[1],getpid());

                //此函数正如名字，将上面封装的结构体放入到线程中的list中，然后等待线程执行该函数返回，最后将list中的task删除
                Stak->sendTask_waitCall_cleanTask(&mework);
                //将线程返回值填入寄存器x0处
                context->uc_mcontext.regs[0] = mework.ret;
            #endif

            //此处一摸一样不进行讲解了，除了获取寄存器方式不同
            #if defined(__arm__)
                sigcontext mcontext = context->uc_mcontext;
                mework.syscall = info->si_syscall;
                mework.parametersNum = 2;
                mework.parameters[0] = (void*)mcontext.arm_r0;
                mework.parameters[1] = (void*)mcontext.arm_r1;

                __android_log_print(ANDROID_LOG_INFO,TAG ,"kill arg:%x   %x   mepid:%x",mework.parameters[0],mework.parameters[1],getpid());

                Stak->sendTask_waitCall_cleanTask(&mework);
                context->uc_mcontext.arm_r0 = mework.ret;
            #endif

            //当你需要支持x86时
            #if defined(__i386__)
            #endif

            //当你需要支持x64时
            #if defined(__x86_64__)
            #endif
        }
        //下面类似函数是一致的就不一一介绍了
        if(info->si_syscall == NT_exit){
            __android_log_print(ANDROID_LOG_INFO,TAG ,"exit sigsys_handler");

            #if defined(__aarch64__)
                sigcontext mcontext = context->uc_mcontext;

                mework.syscall = info->si_syscall;
                mework.parametersNum = 1;
                mework.parameters[0] = (void*)mcontext.regs[0];

                __android_log_print(ANDROID_LOG_INFO,TAG ,"exit arg:%x",mework.parameters[0]);

                Stak->sendTask_waitCall_cleanTask(&mework);
                context->uc_mcontext.regs[0] = mework.ret;
                #endif
        }
        if(info->si_syscall == NT_openat){
            __android_log_print(ANDROID_LOG_INFO,TAG ,"openat sigsys_handler");
            #if defined(__aarch64__)
                sigcontext mcontext = context->uc_mcontext;
                mework.syscall = info->si_syscall;
                mework.parametersNum = 4;
                mework.parameters[0] = (void*)mcontext.regs[0];
                mework.parameters[1] = (void*)mcontext.regs[1];
                mework.parameters[2] = (void*)mcontext.regs[2];
                mework.parameters[3] = (void*)mcontext.regs[3];
                Stak->setTask(&mework);
                Stak->waitCall();
                Stak->cleanTask();
                context->uc_mcontext.regs[0] = mework.ret;
                __android_log_print(ANDROID_LOG_INFO,TAG ,"openat ret:%d",mework.ret);
            #endif
        }
        mu.unlock();
    }
    return;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    __android_log_print(ANDROID_LOG_INFO,TAG ,"patch_svc JNI_OnLoad");

    //使用线程池(在设置seccomp之前创建的线程并不受seccomp机制的影响，所以所有的接收到的svc调用都是用该线程执行后然后返回回来，在给寄存器x0赋值，因为线程资源共享的)
    Stak = new Pthread_pool();

    //设置信号处理回调函数
    struct sigaction sa;
    sa.sa_sigaction = &sigsys_handler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSYS, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    //设置pbf规则，决定hook那些系统调用
    //添加bpf规则方式
    //例子：添加一个read函数的svc 调用信号量为0x3F
    //在文件Task.h中
    //#define NT_exit 0x5e                添加一行                #define NT_exit 0x5e
    //#define NT_kill 0x81                --->                  #define NT_kill 0x81
    //#define NT_openat 0x38                                    #define NT_openat 0x38
    //                                                          #define NT_read 0x3F
    //bpf规则：
    //BPF_STMT(BPF_LD+BPF_W+BPF_ABS,offsetof(struct seccomp_data, nr)),                 BPF_STMT(BPF_LD+BPF_W+BPF_ABS,offsetof(struct seccomp_data, nr)),
    //BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_kill,0,1),                                     BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_kill,0,1),
    //BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),                                         BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),
    //BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_exit,0,1),                         --->        BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_exit,0,1),
    //BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),                                         BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),
    //BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_openat,0,1),                                   BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_openat,0,1),
    //BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),                                         BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),
    //BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_ALLOW)                                         BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_read,0,1),
    //                                                                                  BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),
    //                                                                                  BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_ALLOW)
    //以上即可添加成功
    //然后在信号回调中填写对应的代码，处理即可
    struct sock_filter filter[] = {
            BPF_STMT(BPF_LD+BPF_W+BPF_ABS,offsetof(struct seccomp_data, nr)),
            BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_kill,0,1),
            BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),
            BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_exit,0,1),
            BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),
            BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_openat,0,1),
            BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),
            BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_ALLOW)
    };

    //此处结构体用来设置规则数量和规则的指针
    struct sock_fprog prog = {
            (unsigned short)(sizeof(filter)/sizeof(filter[0])),//规则条数
            filter,                                         //结构体数组指针
    };
    //设置权限收缩，不允许进程再提升权限，此为必须
    prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0);             //必要的，设置NO_NEW_PRIVS
    //开启SECCOMP
    prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&prog);
    return JNI_VERSION_1_6;
}