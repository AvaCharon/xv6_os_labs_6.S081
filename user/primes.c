#include "kernel/types.h"
#include "user.h"

/*
 *对一批数字进行一次质数筛选
 *array:要处理的数组，int：数组长度
 */
void primeFunc(int *array,int len){
	int p[2];
	pipe(p);
    //判断是否到达最后一次筛选：当数组只有一个元素时
	if(len==1){
		printf("prime %d\n",array[0]);
		exit(0);
	}
	if(fork()==0){
		char buf[4];
		close(p[1]);
		int input[len];
		len = 0;
        //不断从管道中读取出上一次筛选剩下的数
		while(read(p[0],buf,4)!=0){
			input[len] = (int)(*buf);
			len++;
		}
		close(p[0]);
        //递归调用函数
		primeFunc(input,len);
		exit(0);
	}
	close(p[0]);
	int point = array[0];
	printf("prime %d\n",point);
	for(int i=1;i<len;i++){
        //如果能被这一轮选中的质数整除，说明不是质数，舍弃
		if(array[i]%point==0){
			continue;
		}else{
            //否则通过管道传输给子进程，进入下一轮筛选
			write(p[1],(char*)(&array[i]),4);
		}
	}
	close(p[1]);
	wait(0);
	exit(0);
}


int main(void){
        int input[34];
        for(int i=0,j=2;i<34;i++,j++){
                input[i]=j;
        }
        primeFunc(input,34);
        exit(0);
}


