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

using namespace std ;

#define MAX_SIZE 255

std::vector<std::string> split(std::string s, const std::string &delimiter);

void executeCommand(const std::vector<std::string>& args);//执行命令函数

void ParsePipe(const std::vector<std::string>& args);//管道函数

void ParseRedirection(const std::vector<std::string>& args);


int main() {
  // 不同步 iostream 和 cstdio 的 buffer
  std::ios::sync_with_stdio(false);

  // 用来存储读入的一行命令
  std::string cmd;
  while (true) {
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

void executeCommand(const std::vector<std::string>& args){
  // 处理外部命令
    pid_t pid = fork();
    // std::vector<std::string> 转 char **
    char *arg_ptrs[args.size() + 1];
    for (auto i = 0; i < args.size(); i++) {
      arg_ptrs[i] = (char*)args[i].c_str();
    }
    // exec p 系列的 argv 需要以 nullptr 结尾
    arg_ptrs[args.size()] = nullptr;

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
      int ret = wait(nullptr);
      if (ret < 0) {
        std::cout << "wait failed";
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
    fd_out = open((*(it_out + 1)).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    /*O_WRONLY：这表示文件只可写。这种模式下，尝试读取文件会导致错误。
    O_CREAT：这意味着如果文件不存在，则创建它。这个标志通常用于确保在试图打开文件时，如果该文件不存在，系统会创建它，而不是报错。
    O_TRUNC：这个标志表示如果文件存在，在打开时将其内容截断到0长度。这意味着文件内容会被清空。这个标志与 O_WRONLY 一起使用时，通常用于确保在写入文件时，旧内容不会保留下来。
    权限模式的前三位分别表示用户（文件所有者）、用户组和其他人的权限。
    6 表示读和写权限 (4 是读权限，2 是写权限)。
    4 表示只有读权限。
    */
   if( fd_out < 0 ) std::cout<<"打开文件失败"<<std::endl ;
  }

  if( it_append != args.end() ){//>>存在
    fd_out = open((*(it_append + 1)).c_str(), O_WRONLY | O_CREAT | O_APPEND, 0777);
    /*O_WRONLY：这表示文件只可写。这种模式下，尝试读取文件会导致错误。
    O_CREAT：这意味着如果文件不存在，则创建它。这个标志通常用于确保在试图打开文件时，如果该文件不存在，系统会创建它，而不是报错。
    O_APPEND：打开文件时，所有写入操作都会自动追加到文件的末尾。
    权限模式的前三位分别表示用户（文件所有者）、用户组和其他人的权限。
    6 表示读和写权限 (4 是读权限，2 是写权限)。
    4 表示只有读权限。
    */
   if( fd_out < 0 ) std::cout<<"打开文件失败"<<std::endl ;
  }

  if( it_in != args.end() ){//<存在
    fd_in = open((*(it_in + 1)).c_str(), O_RDONLY);
    /*O_RDONLY：只读
    */
   if( fd_in < 0 ) std::cout<<"打开文件失败"<<std::endl ;
  }

  // 重定向标准输出
  if (fd_out != -1) {
      dup2(fd_out, STDOUT_FILENO);//对输出进行重定向
      close(fd_out); // 关闭不再需要的文件描述符
  }

  // 重定向标准输入
  if (fd_in != -1) {
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