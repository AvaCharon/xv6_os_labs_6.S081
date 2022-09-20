//仅能每次接收传递一行参数
//ctrl+D结束输入
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


int main(int argc,char *argv[]){
    if(argc < 2){
	    printf("xargs needs more argument!\n"); //检查参数数量是否正确
        exit(-1);
    }
    
    //字符串数组，用于存储命令参数
    //MAXARG=32是exec的最大参数数
    char *args[MAXARG];
    int index = 0;
    for(int j=1;j<argc;j++){
        args[index++]=argv[j];
    }

    //从标准输入中读取追加参数
    char buf[512];
    //存储每行参数，即每次执行命令追加传递的参数
    char line[512];
 
    //使要传递的参数数组指向最后一个参数的指针指向line
    //在for循环中只要每次改变line的值即可实现逐行读取参数
    args[index]=line;

    //由于每接收一行就执行一次对应指令
    //逐行读取
    while(read(0,buf,512)>0){
        //printf("buf: %s f",buf);
        //printf("line: %s f",line);
        //读取到换行符
        //换行符后实际上buf的内容仍是上次读的未被覆盖的内容
        for(int i=0;i<strlen(buf);i++){
            if(buf[i]=='\n'){
                //创建子线程执行一次命令
                if(fork()==0){
                    exec(argv[1],args);
                    exit(0);
                }
                //将line清空
                //memset(line,'\0',512);
                //memset(buf,'\0',512);
                wait(0);
            }else{
                //未到换行则还在将参数读入中
                line[i]=buf[i];
            }
        }
    } 

    exit(0);
}
