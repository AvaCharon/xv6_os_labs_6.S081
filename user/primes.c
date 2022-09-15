#include "kernel/types.h"
#include "user.h"

void primeFunc(int *array,int len){
	int p[2];
	pipe(p);
	if(len==1){
		printf("prime %d\n",array[0]);
		exit(0);
	}
	if(fork()==0){
		char buf[4];
		close(p[1]);
		int input[len];
		len = 0;
		while(read(p[0],buf,4)!=0){
			input[len] = (int)(*buf);
			len++;
		}
		close(p[0]);
		primeFunc(input,len);
		exit(0);
	}
	close(p[0]);
	int point = array[0];
	printf("prime %d\n",point);
	for(int i=1;i<len;i++){
		if(array[i]%point==0){
			continue;
		}else{
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


