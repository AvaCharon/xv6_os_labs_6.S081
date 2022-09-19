#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"



void find(char *path,char *filename){

    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
 
    //open() 系统调用打开文件，0表示打开方式，返回一个文件描述符，如果错误返回-1
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    //fstat()系统调用通过文件描述符fd，返回文件信息到st中,出错时返回-1
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    //如果路径不是目录
    if(st.type!=T_DIR){
        printf("find:%s is not a directory\n",path);
        close(fd);
        return;
    }

    //如果路径过长缓冲区放不下
    //目录的 i 节点的类型 T_DIR, 它的数据是一系列的目录条目。
    //每个条目是一个struct dirent (3700)结构体， 包含一个名字和一个 i 节点编号。
    //这个名字最多有 DIRSIZ (14)个字符
    //如果比较短，它将以 NUL（0）作为结尾字符。
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n"); 
        close(fd);
        return;
    }

    //将path复制到buf中并在最后面加/
    //buf存储的是path目录下文件的绝对路径前缀
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';

    //read将fd中的de大小的字节读到de中，返回读取的字节数
    //de数据结构：  
    //ushort inum：文件编号
    //char name[DIRSIZ]：文件名
    //de即path目录下的每个文件
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0){
            continue;
        }
        //如果正在遍历的是.或..则不进行递归处理
        if((strcmp(de.name,".")==0)||(strcmp(de.name,"..")==0)){
            continue;
        }
        
        //把de.name复制到p，即拼接到buf存储的绝对路径后面
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        //stat()系统调用将已知文件buf的文件信息返回到st中，出错时返回-1
        if(stat(buf, &st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
         }
         
        //如果该文件是目录，递归调用find查找
        if (st.type == T_DIR) {
            find(buf, filename);
        }

        //如果该文件是文件类型并且其名称与要查找的文件名相同
        else if (st.type == T_FILE && (strcmp(de.name, filename)==0)){
            printf("%s\n", buf);
        }

    }
    
    close(fd);
}


int main(int argc, char *argv[])
{
  //检查参数个数
  if(argc != 3){
    printf("Find needs 2 argument!\n");
    exit(1);
  }
  find(argv[1],argv[2]);
  exit(0);
}
