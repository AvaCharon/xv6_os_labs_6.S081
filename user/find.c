#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *dir,char *filename){
    
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
