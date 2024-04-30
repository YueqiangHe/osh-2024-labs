# README

**何跃强 PB22111649**

#### 一、目录导航

1、pwd

使用`getcwd`函数。同时需要考虑报错，即含`pwd`的命令只能有一个字符串

运行结果：

![image-20240430071820573](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430071820573.png)

2、cd

使用`chdir`函数。这样就可以实现打开相应的目录。

运行示例：

![image-20240430072023242](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430072023242.png)

3、【选做】实现cd在没有第二个参数时，默认进入家目录

使用`chdir`函数打开相应的目录，再使用`getenv`搜索 `name` 所指向的环境字符串。

运行示例：

![image-20240430072235777](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430072235777.png)

#### 二、管道

使用`dup2`函数把文件描述符重定向，使用`close`关闭写端，使用`wait()`等待子进程完成。

使用递归实现多个管道。

【注】：1、不需要考虑含有内建命令的管道；2、 `|` 符号的两边总是各有至少一个空格字符。

运行示例：

​	单个管道：

![image-20240430073055048](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430073055048.png)

​	多个管道：

![image-20240430073106556](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430073106556.png)

#### 三、重定向

先检测重定向符号，使用`open`函数写文件，使用`dup2`对输出进行重定向，使用`close`函数关闭文件描述符。

最后再去除重定向符号并执行剩余命令。

【注】：1、不需要考虑含有内建命令的重定向；2、不需要考虑 `stderr` 的重定向；3、重定向符号的两边总是各有至少一个空格字符。

运行示例：

​	支持 `>` 重定向：

![image-20240430073850139](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430073850139.png)

​	支持`>>`  重定向：

![image-20240430073932224](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430073932224.png)

​	支持 `<` 重定向：

![image-20240430073943140](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430073943140.png)

#### 四、信号处理

利用程序中撰写的`setupSigaction()`配置`Sigaction`结构体。使用`signal(SIGTTOU, SIG_IGN);`忽略`SIGTTOU`信号，从而按下`ctrl C`的时候不会退出`shell`程序。

识别型号后，执行撰写的`signalHandler`函数。利用`sigsetjmp(jump_buffer, 1)`设置跳转点，利用`siglongjmp(jump_buffer, 1)`跳转到main程序开头。

运行结果：

![image-20240430074723588](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430074723588.png)

#### 五、前后台进程

设置`background_pid`后台进程组，当最后指令有&的时候，把指令添加到后台，如果使用wait函数，那么我们等待后台指令结束，后台进程组也随之清空。

运行结果：

![image-20240430094304021](C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430094304021.png)

wait之后执行还未结束的sleep 10命令

#### 六、可选功能

支持history :

<img src="C:\Users\Admin\AppData\Roaming\Typora\typora-user-images\image-20240430094749005.png" alt="image-20240430094749005" style="zoom: 80%;" />

可以看出是按照`history，!!，!n`运行正确
