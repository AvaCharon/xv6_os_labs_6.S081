#include "kernel/types.h"
#include "user.h"

int main(void){
	int p1[2];
	int p2[2];
	pipe(p1);
	pipe(p2);
    char ping[4];
    char pong[4]; 
    if(fork()==0){
        //关闭不需要用的管道端
		close(p1[1]);
		close(p2[0]);
        //从管道读取端文件中读取并输出到ping中
		read(p1[0],ping,4);
		close(p1[0]);
		printf("%d: received %s\n",getpid(),&ping);
		write(p2[1],"pong",4);
		close(p2[1]);
		exit(0);
	}else{
		close(p1[0]);
		close(p2[1]);
		write(p1[1],"ping",4);
		close(p1[1]);
		read(p2[0],pong,4);
		close(p2[0]);
		printf("%d: received %s\n",getpid(),&pong);
		wait(0);
	}
	exit(1);
}
