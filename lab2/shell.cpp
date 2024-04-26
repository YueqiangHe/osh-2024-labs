// IO
#include <iostream>
// std::string
#include <string>
// std::vector
#include <vector>
// std::string 转 int
#include <sstream>
// PATH_MAX 等常量
#include <climits>
// POSIX API
#include <unistd.h>
// wait
#include <sys/wait.h>
#include <cstring> // 为 strcat, strcpy, strlen 提供函数支持
#include <algorithm> //为std::find提供头文件
#include <fcntl.h>// open函数需要头文件
#include <csignal> // 为处理信号提供支持
#include <csetjmp> //为信号跳转提供头文件支持

using namespace std ;

#define MAX_SIZE 255


sigjmp_buf jump_buffer;

std::vector<std::string> split(std::string s, const std::string &delimiter);

void executeCommand(const std::vector<std::string>& args);//执行命令函数

void ParsePipe(const std::vector<std::string>& args);//管道函数

void ParseRedirection(const std::vector<std::string>& args);

// 定义全局变量，用于跟踪程序是否收到 Ctrl+C 信号。volatile 是 C/C++ 中的一个关键字，用来告诉编译器某个变量可能会在程序之外的其他地方被修改。。这个地方可能是信号处理程序、硬件中断，或者多线程环境下的其他线程。
volatile sig_atomic_t signal_flag = 0;
// 信号处理函数
void signalHandler(int signal);
// 配置 sigaction
void setupSigaction();
std::vector<pid_t> background_pids;//设置后台进程组并且将所有的进程组数据都记录下来,用来实现进程管理


int main() {
  // 不同步 iostream 和 cstdio 的 buffer
  std::ios::sync_with_stdio(false);

  setupSigaction(); // 配置信号处理，即Sigaction结构体

  // 用来存储读入的一行命令
  std::string cmd;
  while (true) {
    if (sigsetjmp(jump_buffer, 1) == 0){ // 设置跳转点
      // 打印提示符
      std::cout << "$ ";

      
      // 读入一行。std::getline 结果不包含换行符。
      std::getline(std::cin, cmd);

      // 按空格分割命令为单词
      std::vector<std::string> args = split(cmd, " ");

      // 没有可处理的命令
      if (args.empty()) {
        continue;
      }

      // 退出
      if (args[0] == "exit") {
        if (args.size() <= 1) {
          return 0;
        }

        // std::string 转 int
        std::stringstream code_stream(args[1]);
        int code = 0;
        code_stream >> code;

        // 转换失败
        if (!code_stream.eof() || code_stream.fail()) {
          std::cout << "Invalid exit code\n";
          continue;
        }

        return code;
      }

      if (args[0] == "pwd") {
        if( args.size() == 1 ){
          char path[ MAX_SIZE ];
          getcwd( path , sizeof( path ) );//通过getcwd()函数来读取当前路径
          printf( "%s\n",path );
          continue;
        }
        else{
          std::cout<<"Input Fault"<<std::endl ;//输入错误
          continue;
        }
      }

      if (args[0] == "cd") {
        if( args.size() == 2){
          chdir( args[1].c_str() );
        }
        else{
          std::cout<<"Input Fault"<<std::endl ;//输入格式错误
        }
        continue;
      }

      //处理wait内置命令
      if( args[0] == "wait"){
        for( auto i = 0 ; i < background_pids.size() ; i++ ){
          wait(nullptr);//等待后台进程结束
        }
        background_pids.clear();//后台命令完全结束之后将后台命令完全清空
        continue ;
      }

      // 保存原始的标准输入/输出文件描述符
      int original_stdin = dup(STDIN_FILENO);
      int original_stdout = dup(STDOUT_FILENO);
      ParsePipe( args );
      // 重置标准输入/输出
      dup2(original_stdin, STDIN_FILENO);
      dup2(original_stdout, STDOUT_FILENO);

      // 关闭临时文件描述符
      close(original_stdin);
      close(original_stdout);
      //只有这样才能让输入/输出流没有问题，这样才可以正确使用getline()
    }else{
      // 如果跳转回来，表示收到 SIGINT
      std::cout << "\n"; // 表示捕获了信号
      continue ;
    }
  }
}


// 经典的 cpp string split 实现
// https://stackoverflow.com/a/14266139/11691878
std::vector<std::string> split(std::string s, const std::string &delimiter) {
  std::vector<std::string> res;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    res.push_back(token);
    s = s.substr(pos + delimiter.length());
  }
  res.push_back(s);
  return res;
}

void executeCommand(const std::vector<std::string>& args){//根据原本的框架复制的代码
    //处理&
    bool have_background = false ;
    std::vector<std::string> new_args = args ;
    if(new_args.back() == "&"){
      have_background = true ;
      new_args.erase( new_args.end()-1 , new_args.end() );//删除&
    }
    
    // 处理外部命令
    pid_t pid = fork();
    // std::vector<std::string> 转 char **
    char *arg_ptrs[new_args.size() + 1];
    for (auto i = 0; i < new_args.size(); i++) {
      arg_ptrs[i] = (char*)new_args[i].c_str();
    }
    // exec p 系列的 argv 需要以 nullptr 结尾
    arg_ptrs[new_args.size()] = nullptr;

    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
      // 这里只有子进程才会进入
      // execvp 会完全更换子进程接下来的代码，所以正常情况下 execvp 之后这里的代码就没意义了
      // 如果 execvp 之后的代码被运行了，那就是 execvp 出问题了
      execvp(args[0].c_str(), arg_ptrs);

      // 所以这里直接报错
      exit(255);
    }else if( pid > 0 ){
      // 这里只有父进程（原进程）才会进入
      if( have_background ){//只有最后有&才需要
        background_pids.push_back(pid);  // 将此进程加入后台进程
      }else{
        // 等待子进程完成
        pid_t ret = wait(nullptr); // 使用 wait 等待子进程结束
        if (ret < 0) {
          std::cout << "\nwait failed\n";
        }
      }
    }
}

void ParsePipe(const std::vector<std::string>& args){
  auto it = std::find( args.begin() , args.end() , "|");//利用迭代器查找第一个管道符号
  if( it == args.end() ){
    ParseRedirection( args );//没有管道符号继续执行重定向
    return ;
  }

  //有管道符号，那么可以定义左命令和右命令
  std::vector<std::string> left(args.begin(), it);
  std::vector<std::string> right(it + 1, args.end());
  

  int fd[2] ;
  if( pipe(fd) == -1 ){
    std::cout<<"管道创造失败"<<std::endl;
    return ;
  }

  pid_t pid = fork();//创建子进程

  if( pid == 0 ){//子进程
    close( fd[0] );//关闭子进程读端
    dup2( fd[1] , STDOUT_FILENO );//使用dup2函数将STDOUT_FINLENO(这个宏在unitstd.h定义，为１)这个文件描述符重定向到了连接套接字：dup2(connfd, STDOUT_FILENO)。
    close( fd[1] ) ;//关闭管道的写端。在重定向标准输出之后，写端文件描述符不再需要，所以需要关闭以释放资源。
    executeCommand( left );//执行左边的程序
    exit(0);
  }else{//父进程
    close( fd[1] );//关闭写端
    wait(nullptr); // 等待子进程完成
    dup2( fd[0] , STDIN_FILENO );//使用dup2函数将STDIN_FINLENO(这个宏在unitstd.h定义，为１)这个文件描述符重定向到了连接套接字：dup2(connfd, STDOUT_FILENO)。
    close( fd[0] ) ;//关闭管道的读端。在重定向标准输出之后，写端文件描述符不再需要，所以需要关闭以释放资源。
    ParsePipe(right);//递归实现多个管道
  }
}

void ParseRedirection(const std::vector<std::string>& args){
  auto it_out = std::find( args.begin() , args.end() , ">");//利用迭代器查找>
  auto it_append = std::find( args.begin() , args.end() , ">>");//利用迭代器查找>>
  auto it_in = std::find( args.begin() , args.end() , "<");//利用迭代器查找<

  int fd_in = -1 ;//输出文件描述符
  int fd_out = -1 ;//输出文件描述符

  if( it_out != args.end() ){//>存在
    fd_out = open((*(it_out + 1)).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 777);
    /*O_WRONLY：这表示文件只可写。这种模式下，尝试读取文件会导致错误。
    O_CREAT：这意味着如果文件不存在，则创建它。这个标志通常用于确保在试图打开文件时，如果该文件不存在，系统会创建它，而不是报错。
    O_TRUNC：这个标志表示如果文件存在，在打开时将其内容截断到0长度。这意味着文件内容会被清空。这个标志与 O_WRONLY 一起使用时，通常用于确保在写入文件时，旧内容不会保留下来。
    权限模式的前三位分别表示用户（文件所有者）、用户组和其他人的权限。
    */
   if( fd_out < 0 ) std::cout<<"打开文件失败"<<std::endl ;
  }

  if( it_append != args.end() ){//>>存在
    fd_out = open((*(it_append + 1)).c_str(), O_WRONLY | O_CREAT | O_APPEND, 777);
    /*O_WRONLY：这表示文件只可写。这种模式下，尝试读取文件会导致错误。
    O_CREAT：这意味着如果文件不存在，则创建它。这个标志通常用于确保在试图打开文件时，如果该文件不存在，系统会创建它，而不是报错。
    O_APPEND：打开文件时，所有写入操作都会自动追加到文件的末尾。
    权限模式的前三位分别表示用户（文件所有者）、用户组和其他人的权限。
    */
   if( fd_out < 0 ) std::cout<<"打开文件失败"<<std::endl ;
  }

  if( it_in != args.end() ){//<存在
    fd_in = open((*(it_in + 1)).c_str(), O_RDONLY);
    /*O_RDONLY：只读*/
   if( fd_in < 0 ) std::cout<<"打开文件失败"<<std::endl ;
  }

  // 重定向标准输出
  if (fd_out != -1) {//需要输出
      dup2(fd_out, STDOUT_FILENO);//对输出进行重定向
      close(fd_out); // 关闭不再需要的文件描述符
  }

  // 重定向标准输入
  if (fd_in != -1) {//需要输入
      dup2(fd_in, STDIN_FILENO);//对输入进行重定向
      close(fd_in); // 关闭不再需要的文件描述符
  }

  //下面对命令进行修剪从而让剩下的程序执行
  std::vector<std::string> newcmd = args ; 

  //需要重新赋值newcmd,因为之前是在args地址下进行的，所以会导致segementation fault
  it_out = std::find(newcmd.begin(), newcmd.end(), ">");
  if( it_out != newcmd.end() ){//>存在
    newcmd.erase( it_out , it_out+2 ) ;//移除重定向的部分得到没有重定向的命令
  }

  it_append = std::find(newcmd.begin(), newcmd.end(), ">>");
  if( it_append != newcmd.end() ){//>>存在
    newcmd.erase( it_append , it_append+2 ) ;//移除重定向的部分得到没有重定向的命令
  }

  it_in = std::find(newcmd.begin(), newcmd.end(), "<");
  if( it_in != newcmd.end() ){//<存在
    newcmd.erase( it_in , it_in+2 ) ;//移除重定向的部分得到没有重定向的命令
  }

  executeCommand( newcmd );
}

void signalHandler( int signal ){
  // 终止所有子进程
  if (signal == SIGINT) {
        siglongjmp(jump_buffer, 1);//利用跳转函数捕获后直接跳转到相应地方
    }
}

//设置 SIGINT 信号的处理方式，即配置Sigaction结构体。
void setupSigaction(){
  /*
  sigaction结构体成员：
  sa_handler成员：
  对捕获的信号进行处理的函数，函数参数为sigaction函数的参数1信号（概念上等同于单独使用signal函数）
  也可以设置为后面两个常量：常数SIG_IGN(向内核表示忽略此信号)或是常数SIG_DFL(表示接到此信号后的动作是系统默认动作)
  sa_mask成员：
  功能：sa_mask是一个信号集，当接收到某个信号，并且调用sa_handler函数对信号处理之前，把该信号集里面的信号加入到进程的信号屏蔽字当中，当sa_handler函数执行完之后，这个信号集中的信号又会从进程的信号屏蔽字中移除
  为什么这样设计？？这样保证了当正在处理一个信号时，如果此种信号再次发生，信号就会阻塞。如果阻塞期间产生了多个同种类型的信号，那么当sa_handler处理完之后。进程又只接受一个这种信号
  即使没有信号需要屏蔽，也要初始化这个成员（sigemptyset()），不能保证sa_mask=0会做同样的事情
  sigset_t数据类型见文章：https://blog.csdn.net/qq_41453285/article/details/89228297
  sa_restorer成员：
  已经被抛弃了，不再使用
  sa_flags成员：
  指定了对信号进行哪些特殊的处理
  */
  struct sigaction action ;//用于指定信号处理的相关信息。
  signal(SIGTTOU, SIG_IGN);//忽略 SIGTTOU 信号。使用 SIG_IGN 可以让程序忽略特定信号。通常用于信号可能导致程序不必要中断或终止的场景。
  std::memset(&action, 0, sizeof(action)); // 清空结构体,防止旧值影响
  action.sa_handler = signalHandler ;//设置信号处理函数为signalHandler，这样就可以处理SIGINT信号的函数了
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0 ;//信号处理结构的其他选项没有特殊标志
  sigaction(SIGINT, &action, nullptr);
  /*
  sigaction函数：
  参数1：要捕获的信号
  参数2：接收到信号之后对信号进行处理的结构体
  参数3：接收到信号之后，保存原来对此信号处理的各种方式与信号（可用来做备份）。如果不需要备份，此处可以填NULL
  */
}

