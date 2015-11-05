#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>  
#include "MPI_node.h"

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64
#define DIR   "/home/fangling/ELM_SMP/sample/"
#define CORE_NUM 100

struct Node_sample{
	int fd[CORE_NUM];
	int node_id;
};
struct Node_sample ns[MAX_NODE];


int file_size(char* filename)  
{  
    struct stat statbuf;  
    stat(filename,&statbuf);  
    int size=statbuf.st_size;  
  
    return size;  
}  

int main(int argc,char **argv)
{
	int fd_src;
	int i,k,j,rank,count;
	char temp[10],buf[128];
	char command[100];
	char node_name[100];
	int node_num = 0;
	int proc_num = 0,proc_size;					//总进程数
	int average_data ;
	int allfd[100];
	int num;
	if(argc != 3){
		printf("must three arguments!!!\n");
		exit(EXIT_FAILURE);
	}
	//得到进程数和平均的行数
	proc_num = atoi(argv[1]);
	average_data = file_size(argv[2])/proc_num;
	
	//初始化节点与进程的拓扑
	MPIN_init("../hostfile",proc_num);
	node_num = MPIN_get_node_size();
	
	if((fd_src = open(argv[2],O_RDONLY)) == -1){
		perror("open1");
		exit(EXIT_FAILURE);
	}
	k = 0;
	for(i = 0;i< node_num;i++)
	{
		ns[i].node_id = i;
		proc_size = MPIN_get_node_process_size(i);
		for(j = 0;j < proc_size;j++){
			rank = MPIN_get_node_process_rank(i,j);
			sprintf(temp,"%d",rank);
			if((ns[i].fd[j] = open(temp,O_CREAT|O_WRONLY,0777)) == -1){
				perror("open2");
				exit(EXIT_FAILURE);
			}
			allfd[k++] = ns[i].fd[j];
		}
	}

	//从大文件中读数据
	k = 0;
	count = 0;
	do{
		num = read(fd_src,buf,1);
		write(allfd[k],buf,num);
		count += num;
		if(count > average_data){
			k++;
			count = 0;
		}
		if(k >= proc_num){
			break;
		}
	}while(num == 1);
	
	close(fd_src);
	for(i = 0;i< node_num;i++)
	{
		proc_size = MPIN_get_node_process_size(i);
		for(j = 0;j < proc_size;j++){
			close(ns[i].fd[j]);
		}
	}
	
	//将文件分发到对应节点
	for(i = 0;i< node_num;i++)
	{
		proc_size = MPIN_get_node_process_size(i);
		for(j = 0;j < proc_size;j++){
			rank = MPIN_get_node_process_rank(i,j);
			sprintf(command,"scp %s%d fangling@%s:%s",DIR,rank,arraynode[i].name,DIR);
			printf("%s\n",command);
			//system(command);
		}
	}
	
	return 0;
}
