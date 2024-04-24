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
using namespace std ;

#define MAX_SIZE 255

std::vector<std::string> split(std::string s, const std::string &delimiter);

void executeCommand(const std::vector<std::string>& args);//执行命令函数

void ParsePipe(const std::vector<std::string>& args);//管道函数


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

    pid_t pid ;
    auto it = std::find( args.begin() , args.end() , "|");//利用迭代器查找第一个管道符号
    if( it != args.end() ){
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
      continue;
    }


    // 处理外部命令
    pid = fork();

    // std::vector<std::string> 转 char **
    char *arg_ptrs[args.size() + 1];
    for (auto i = 0; i < args.size(); i++) {
      arg_ptrs[i] = &args[i][0];
    }
    // exec p 系列的 argv 需要以 nullptr 结尾
    arg_ptrs[args.size()] = nullptr;

    if (pid == 0) {
      // 这里只有子进程才会进入
      // execvp 会完全更换子进程接下来的代码，所以正常情况下 execvp 之后这里的代码就没意义了
      // 如果 execvp 之后的代码被运行了，那就是 execvp 出问题了
      execvp(args[0].c_str(), arg_ptrs);

      // 所以这里直接报错
      exit(255);
    }

    // 这里只有父进程（原进程）才会进入
    int ret = wait(nullptr);
    if (ret < 0) {
      std::cout << "wait failed";
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
    executeCommand( args );//没有管道符号直接执行
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
    executeCommand( left );
    exit(0);
  }else{
    close( fd[1] );//关闭写端
    wait(nullptr); // 等待子进程完成
    dup2( fd[0] , STDIN_FILENO );//使用dup2函数将STDIN_FINLENO(这个宏在unitstd.h定义，为１)这个文件描述符重定向到了连接套接字：dup2(connfd, STDOUT_FILENO)。
    close( fd[0] ) ;//关闭管道的读端。在重定向标准输出之后，写端文件描述符不再需要，所以需要关闭以释放资源。
    ParsePipe(right);//递归实现多个管道
  }
}