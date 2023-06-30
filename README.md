# Hook_Android
该项目可以hook Android上so层代码，以及hook svc调用<br>

该项目在hook so时，使用在app_process64或app_process32中添加一个动态链接库的方式<br>
patchelf --add-needed libmehook.so app_process64<br>
patchelf --add-needed libmehook.so app_process32<br>
使用该命令添加，需要安装相关库。<br>
然后使用cp替换掉文件<br>

常规hook so<br>
MyHook.cpp编译后得到libmehook.so<br>
patch_arm.cpp编译后得到libpatch_arm.so<br>
将libmehook.so放入系统库以便执行app_process64或app_process32可以正确加载进入，在libmehook.so中hook了RegisterNatives函数，此函数处获取到setProcessPackageName。<br>
在这个函数中可以拿到程序的包名，拿到程序报名后进行匹配，如果是要hook的程序则进行注入so，将libpatch_arm.so注入进程序中，所有的对程序so hook都是在此函数中进行。<br>
如果有其他工具可以直接load libpatch_arm.so即可。比如xposed<br>
对于包名匹配后期进行改进，现在暂时不动他。<br>

hook svc<br>
此方法使用看雪介绍的方案<br>
利用seccomp机制进行<br>
代码首先利用seccomp机制缩进权限，然后指定bpf规则，已经写好了3条规则，规则中表示当得到的系统调用号为NT_kill时，则进入陷阱此时会发送信号给程序。<br>
代码通过一个在设置seccomp之前的线程用来执行信号会调用拿到的svc相关信息，然后在线程中将得到的返回值在填写到信号回调中的寄存器中返回。<br>
bpf其他规则直接仿照即可<br>
比如：write<br>
BPF_STMT(BPF_LD+BPF_W+BPF_ABS,offsetof(struct seccomp_data, nr)),<br>
BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_kill,0,1),<br>
BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),<br>
BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_exit,0,1),<br>
BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),<br>
BPF_JUMP(BPF_JMP+BPF_JEQ| BPF_K,NT_write,0,1),<br>
BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_TRAP),<br>
BPF_STMT(BPF_RET+BPF_K,SECCOMP_RET_ALLOW),<br>
这样写即可，然后需要在信号回调函数处处理，打印出相关日志，ucontext参数为环境上下文可以拿到寄存器信息。<br>
回调函数为sigsys_handler<br>
该so为libpatch_svc.so,注入可以选择使用libmehook.so也可以使用xposed<br>
暂时的缺陷为只可以hook到svc之前的而无法hook到svc之后的,并且测试中发现在游戏中播放广告会广告崩溃日志显示是由于内存紧张杀死的程序，我有些担心是因为线程中的死循环，或者是由权限收缩导致的，后期进行改进<br>
