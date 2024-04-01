#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>


int main(){
		char buf1[1];
		char buf2[100];
		long int ret1 = syscall(548 , buf1, 1);
		long int ret2 = syscall(548 , buf2, 100);
		printf("buf_size:1, return:%ld, buf:%s\n" ,ret1, buf1);
		printf("buf_size:100, return:%ld, buf:%s\n" ,ret2, buf2);

								

							
		while(1){}
}
