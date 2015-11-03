#include <stdio.h>
#include <stdlib.h>
#include "MPI_node.h"
#include "matrix.h"
#include "alg.h"
#include "mpi.h"
/*
	根据总的节点数建立一个Dtree拓扑结构
*/
void create_dtree(int size,int d){
	int i,j;
	for(i = 0;i< size;i++){
		dt[i].id = i;
		if(i == 0)
			dt[i].parent = -1;
		else{
			dt[i].parent = (i-1)/d;
		}
		struct child *s;
		dt[i].next = (struct child *)malloc(sizeof(struct child));
		s = dt[i].next;
		s->next = NULL;
		for(j = 1;j <= d;j++)
		{
			struct child *next = (struct child *)malloc(sizeof(struct child));
			if(i*d+j < size){
				next->id = i*d+j;	
			}else{
				next->id = -1;
			}
			s->next = next;
			s = s->next;
			s->next = NULL;
		}	
	}
}
/*
	以node_id为虚拟进程号，按照node_id号
	进行以二叉树算法进行归约
*/
int MPIN_Reduce(float *sendbuf,float *recvbuf,int count,int cur_node_id,int cur_rank,int tag)
{
	MPI_Status status;
	struct child *c;
	int rank;
	int size = MPIN_get_node_size();
	if(cur_node_id >= size){
		return -1;
	}
	c = dt[cur_node_id].next->next;
	//从子进程接收数据
	while(c){
		if(c->id != -1){//如果当前节点有孩子
			rank = MPIN_get_master_rank(cur_node_id);
			MPI_Recv(recvbuf,count,MPI_FLOAT,rank,tag,MPI_COMM_WORLD,&status);
			AddMatrix(recvbuf,sendbuf,count);
		}else{
			break;
		}
		c = c->next;
	}
	int parent_rank = dt[cur_node_id].parent;
	if(parent_rank != -1) //不是根进程
	{
		MPI_Send(sendbuf,count,MPI_FLOAT,parent_rank,tag,MPI_COMM_WORLD);
	}
	return 0;
}
