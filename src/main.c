#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mpi.h"
#include "elm.h"
#include "matrix.h"
#include "MPI_node.h"
#include "alg.h"

int main(int argc,char **argv){

	int m;
	int rank,size; 
	int node_id,master_rank;
	MPI_Status status;
	double starttime,endtime;
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPIN_init("./hostfile",size);
	//创建节点间拓扑结构
	//create_dtree(3,2);
	//将样本集均等划分，主进程不处理样本，子进程处理样本
	m = DATASET/size;
	//printf(" m = %d\n",m);
	node_id = MPIN_get_node_by_rank(rank);
	master_rank = MPIN_get_master_rank(node_id);
	if(rank == 0){
		char dir[20];
		int i,j = 0,k = 0,node_id,node_size,process_num,child_rank,node_master_rank;
		float *Ht,*Hh,*tempht,*temphh,*result,*train_set,*T,*input,*tempI,*tranpH;
		train_set = (float *)calloc(m * NUMROWS,sizeof(float)); 
		T = (float *)calloc(m * OUTPUT_NEURONS,sizeof(float)); 					/* m * OUTPUT_NEURONS */
		input = (float *)calloc(m * INPUT_NEURONS,sizeof(float)); 
		tempI = (float *)calloc(m * HIDDEN_NEURONS,sizeof(float)); 				/* m * HIDDEN_NEURONS */
		result = (float *)calloc(HIDDEN_NEURONS * OUTPUT_NEURONS,sizeof(float));	/* HIDDEN_NEURONS * OUTPUT_NEURONS */
		Ht = (float *)calloc(HIDDEN_NEURONS * OUTPUT_NEURONS,sizeof(float)); 		/* HIDDEN_NEURONS * OUTPUT_NEURONS */
		tempht = (float *)calloc(HIDDEN_NEURONS * OUTPUT_NEURONS,sizeof(float)); 	/* HIDDEN_NEURONS * OUTPUT_NEURONS */
		Hh = (float *)calloc(HIDDEN_NEURONS * HIDDEN_NEURONS,sizeof(float)); 		/* HIDDEN_NEURONS * HIDDEN_NEURONS */
		temphh = (float *)calloc(HIDDEN_NEURONS * HIDDEN_NEURONS,sizeof(float)); 	/* HIDDEN_NEURONS * HIDDEN_NEURONS */
		tranpH = (float *)calloc(HIDDEN_NEURONS * m,sizeof(float)); 				/* m * HIDDEN_NEURONS */

		printf("begin tranning...\n");
		starttime = MPI_Wtime();
		//初始化
		/*权重在计算过程中需要进行转置，因为参数全是随机，所以为了方便计算，
		开始定义就进行转置，原行列为HIDDEN_NEURONS*INPUT_NEURONS*/
		float *weight = (float *)calloc(INPUT_NEURONS*HIDDEN_NEURONS,sizeof(float)); 	/*INPUT_NEURONS * HIDDEN_NEURONS */
		float *bias = (float *)calloc(HIDDEN_NEURONS,sizeof(float)); 
		RandomWeight_s(weight,INPUT_NEURONS,HIDDEN_NEURONS);
		SaveMatrix_s(weight,"./result/weight",INPUT_NEURONS,HIDDEN_NEURONS);	
		RandomBiase(bias,HIDDEN_NEURONS);
		SaveMatrix_s(bias,"./result/bias",1,HIDDEN_NEURONS);
		//随机权重和偏置，并将权重和偏置广播给所有子进程
		MPI_Bcast(weight,INPUT_NEURONS * HIDDEN_NEURONS,MPI_FLOAT,0,MPI_COMM_WORLD);
		MPI_Bcast(bias,HIDDEN_NEURONS,MPI_FLOAT,0,MPI_COMM_WORLD);
		/*InitMatrix(Ht,HIDDEN_NEURONS,OUTPUT_NEURONS);
		InitMatrix(Hh,HIDDEN_NEURONS,HIDDEN_NEURONS);*/
		sprintf(dir,"./sample/%d",rank);
		printf("direction = %s\n",dir);
		//从本地读取相应的样本集
		if(LoadMatrix_s(train_set,dir,m,NUMROWS,1) == 0){
			printf("rank %d:load input file error!!!\n",rank);
			MPI_Abort(MPI_COMM_WORLD,-1);
		}
		/*将数据集划分成输入和输出*/
		for(i = 0;i<m*NUMROWS;i++){
			if(i % NUMROWS == 0){
				T[k++] = train_set[i];
			}else{
				input[j++] = train_set[i];
			}
		}
		//input * weight + B;
		MultiplyMatrix_cblas_s(input,m,INPUT_NEURONS,weight,INPUT_NEURONS,HIDDEN_NEURONS,tempI);
		AddMatrix_bais_s(tempI,bias,m,HIDDEN_NEURONS);
		//sigmoid
		SigmoidHandle_s(tempI,m,HIDDEN_NEURONS);
		TranspositionMatrix_s(tempI,tranpH,m,HIDDEN_NEURONS);
		//H'H
		MultiplyMatrix_cblas_s(tranpH,HIDDEN_NEURONS,m,T,m,OUTPUT_NEURONS,Ht);
		//HT
		MultiplyMatrix_cblas_s(tranpH,HIDDEN_NEURONS,m,tempI,m,HIDDEN_NEURONS,Hh);
		process_num = MPIN_get_node_process_size(node_id);
		for(i = 1;i < process_num;i++){
			child_rank = MPIN_get_node_process_rank(node_id,i);
			MPI_Recv(temphh,HIDDEN_NEURONS * HIDDEN_NEURONS,MPI_FLOAT,child_rank,0,MPI_COMM_WORLD,&status);
			MPI_Recv(tempht,HIDDEN_NEURONS * OUTPUT_NEURONS,MPI_FLOAT,child_rank,1,MPI_COMM_WORLD,&status);
			AddMatrix(tempht,Ht,HIDDEN_NEURONS * OUTPUT_NEURONS);
			AddMatrix(temphh,Hh,HIDDEN_NEURONS * HIDDEN_NEURONS);
		}
		//用二叉树算法进行节点间的归约
		//MPIN_Reduce(Ht,tempht,HIDDEN_NEURONS * OUTPUT_NEURONS,node_id,rank,0);
		//MPIN_Reduce(Hh,temphh,HIDDEN_NEURONS * HIDDEN_NEURONS,node_id,rank,1);
		node_size = MPIN_get_node_size();
		for(i = 1;i<node_size;i++)
		{
			node_master_rank = MPIN_get_master_rank(i);
			MPI_Recv(temphh,HIDDEN_NEURONS * HIDDEN_NEURONS,MPI_FLOAT,node_master_rank,0,MPI_COMM_WORLD,&status);
			MPI_Recv(tempht,HIDDEN_NEURONS * OUTPUT_NEURONS,MPI_FLOAT,node_master_rank,1,MPI_COMM_WORLD,&status);
			AddMatrix(tempht,Ht,HIDDEN_NEURONS * OUTPUT_NEURONS);
			AddMatrix(temphh,Hh,HIDDEN_NEURONS * HIDDEN_NEURONS);
		}
		//1、H'H累加的结果求解其逆
		//岭回归
		for(i = 0;i < HIDDEN_NEURONS*HIDDEN_NEURONS;i++){
				if(i % HIDDEN_NEURONS == i / HIDDEN_NEURONS) 
					Hh[i] += LUMMA; 
		}
		InverseMatirx_cblas_s(Hh,HIDDEN_NEURONS);
		//2、将上面两个结果相乘得到最终结果
		MultiplyMatrix_cblas_s(Hh,HIDDEN_NEURONS,HIDDEN_NEURONS,Ht,HIDDEN_NEURONS,OUTPUT_NEURONS,result);
		endtime = MPI_Wtime();
		printf("finish trainning...\n");
		printf("use time :%f \n",endtime - starttime);
		//回归准确率测试
		SaveMatrix_s(result,"./result/result",HIDDEN_NEURONS,OUTPUT_NEURONS);	
	}else if(rank == master_rank){
			int i,j = 0,k = 0;
			int process_num,child_rank;
			char dir[20];
			float *train_set,*T,*input,*weight,*bias,*tempI,*Ht,*Hh,*tranpH,*tempht,*temphh;
			train_set = (float *)calloc(m * NUMROWS,sizeof(float)); 				/* m * NUMROWS */
			T = (float *)calloc(m * OUTPUT_NEURONS,sizeof(float)); 					/* m * OUTPUT_NEURONS */
			input = (float *)calloc(m * INPUT_NEURONS,sizeof(float)); 				/* m * INPUT_NEURONS */
			weight = (float *)calloc(INPUT_NEURONS * HIDDEN_NEURONS,sizeof(float)); 		/* INPUT_NEURONS * HIDDEN_NEURONS */
			bias = (float *)calloc(HIDDEN_NEURONS,sizeof(float)); 					/* HIDDEN_NEURONS */
			tempI = (float *)calloc(m * HIDDEN_NEURONS,sizeof(float)); 				/* m * HIDDEN_NEURONS */
			Ht = (float *)calloc(HIDDEN_NEURONS * OUTPUT_NEURONS,sizeof(float)); 			/* HIDDEN_NEURONS * OUTPUT_NEURONS */
			Hh = (float *)calloc(HIDDEN_NEURONS * HIDDEN_NEURONS,sizeof(float)); 			/* HIDDEN_NEURONS * HIDDEN_NEURONS */
			tranpH = (float *)calloc(HIDDEN_NEURONS * m,sizeof(float)); 				/* m * HIDDEN_NEURONS */
			tempht = (float *)calloc(HIDDEN_NEURONS * OUTPUT_NEURONS,sizeof(float)); 		/* HIDDEN_NEURONS * OUTPUT_NEURONS */
			temphh = (float *)calloc(HIDDEN_NEURONS * HIDDEN_NEURONS,sizeof(float)); 		/* HIDDEN_NEURONS * HIDDEN_NEURONS */
			MPI_Bcast(weight,(INPUT_NEURONS)*(HIDDEN_NEURONS),MPI_FLOAT,0,MPI_COMM_WORLD);
			MPI_Bcast(bias,HIDDEN_NEURONS,MPI_FLOAT,0,MPI_COMM_WORLD);
			sprintf(dir,"./sample/%d",rank);
			printf("direction = %s\n",dir);
			//从本地读取相应的样本集
			if(LoadMatrix_s(train_set,dir,m,NUMROWS,1) == 0){
				printf("rank %d:load input file error!!!\n",rank);
				MPI_Abort(MPI_COMM_WORLD,-1);
			}
			/*将数据集划分成输入和输出*/
			for(i = 0;i<m*NUMROWS;i++){
				if(i % NUMROWS == 0){
					T[k++] = train_set[i];
				}else{
					input[j++] = train_set[i];
				}
			}
			//input * weight + B;
			MultiplyMatrix_cblas_s(input,m,INPUT_NEURONS,weight,INPUT_NEURONS,HIDDEN_NEURONS,tempI);
			AddMatrix_bais_s(tempI,bias,m,HIDDEN_NEURONS);
			//sigmoid
			SigmoidHandle_s(tempI,m,HIDDEN_NEURONS);
			TranspositionMatrix_s(tempI,tranpH,m,HIDDEN_NEURONS);
			//H'H
			MultiplyMatrix_cblas_s(tranpH,HIDDEN_NEURONS,m,T,m,OUTPUT_NEURONS,Ht);
			//HT
			MultiplyMatrix_cblas_s(tranpH,HIDDEN_NEURONS,m,tempI,m,HIDDEN_NEURONS,Hh);
			//先接收本地的子进程发来的数据并累加
			process_num = MPIN_get_node_process_size(node_id);
			for(i = 1;i < process_num;i++){
				child_rank = MPIN_get_node_process_rank(node_id,i);
				MPI_Recv(temphh,HIDDEN_NEURONS * HIDDEN_NEURONS,MPI_FLOAT,child_rank,0,MPI_COMM_WORLD,&status);
				MPI_Recv(tempht,HIDDEN_NEURONS * OUTPUT_NEURONS,MPI_FLOAT,child_rank,1,MPI_COMM_WORLD,&status);
				AddMatrix(tempht,Ht,HIDDEN_NEURONS * OUTPUT_NEURONS);
				AddMatrix(temphh,Hh,HIDDEN_NEURONS * HIDDEN_NEURONS);
			}
			//再将累加结果在节点间二叉树算法进行归约
			//MPIN_Reduce(Ht,tempht,HIDDEN_NEURONS * OUTPUT_NEURONS,node_id,rank,0);
			//MPIN_Reduce(Hh,temphh,HIDDEN_NEURONS * HIDDEN_NEURONS,node_id,rank,1);
			MPI_Send(Hh,HIDDEN_NEURONS * HIDDEN_NEURONS,MPI_FLOAT,0,0,MPI_COMM_WORLD);
			MPI_Send(Ht,HIDDEN_NEURONS * OUTPUT_NEURONS,MPI_FLOAT,0,1,MPI_COMM_WORLD);
		}else{
			int i,j = 0,k = 0;
			char dir[20];
			float *train_set,*T,*input,*weight,*bias,*tempI,*Ht,*Hh,*tranpH;
			train_set = (float *)calloc(m * NUMROWS,sizeof(float)); 				/* m * NUMROWS */
			T = (float *)calloc(m * OUTPUT_NEURONS,sizeof(float)); 					/* m * OUTPUT_NEURONS */
			input = (float *)calloc(m * INPUT_NEURONS,sizeof(float)); 				/* m * INPUT_NEURONS */
			weight = (float *)calloc(INPUT_NEURONS * HIDDEN_NEURONS,sizeof(float)); 		/* INPUT_NEURONS * HIDDEN_NEURONS */
			bias = (float *)calloc(HIDDEN_NEURONS,sizeof(float)); 					/* HIDDEN_NEURONS */
			tempI = (float *)calloc(m * HIDDEN_NEURONS,sizeof(float)); 				/* m * HIDDEN_NEURONS */
			Ht = (float *)calloc(HIDDEN_NEURONS * OUTPUT_NEURONS,sizeof(float)); 			/* HIDDEN_NEURONS * OUTPUT_NEURONS */
			Hh = (float *)calloc(HIDDEN_NEURONS * HIDDEN_NEURONS,sizeof(float)); 			/* HIDDEN_NEURONS * HIDDEN_NEURONS */
			tranpH = (float *)calloc(HIDDEN_NEURONS * m,sizeof(float)); 				/* m * HIDDEN_NEURONS */

			MPI_Bcast(weight,(INPUT_NEURONS)*(HIDDEN_NEURONS),MPI_FLOAT,0,MPI_COMM_WORLD);
			MPI_Bcast(bias,HIDDEN_NEURONS,MPI_FLOAT,0,MPI_COMM_WORLD);

			sprintf(dir,"./sample/%d",rank);
			printf("direction = %s\n",dir);
			//从本地读取相应的样本集
			if(LoadMatrix_s(train_set,dir,m,NUMROWS,1) == 0){
				printf("rank %d:load input file error!!!\n",rank);
				MPI_Abort(MPI_COMM_WORLD,-1);
			}
			
			/*将数据集划分成输入和输出*/
			for(i = 0;i<m*NUMROWS;i++){
				if(i % NUMROWS == 0){
					T[k++] = train_set[i];
				}else{
					input[j++] = train_set[i];
				}
			}
			//input * weight + B;
			MultiplyMatrix_cblas_s(input,m,INPUT_NEURONS,weight,INPUT_NEURONS,HIDDEN_NEURONS,tempI);
			AddMatrix_bais_s(tempI,bias,m,HIDDEN_NEURONS);
			//sigmoid
			SigmoidHandle_s(tempI,m,HIDDEN_NEURONS);
			TranspositionMatrix_s(tempI,tranpH,m,HIDDEN_NEURONS);
			//H'H
			MultiplyMatrix_cblas_s(tranpH,HIDDEN_NEURONS,m,T,m,OUTPUT_NEURONS,Ht);
			//HT
			MultiplyMatrix_cblas_s(tranpH,HIDDEN_NEURONS,m,tempI,m,HIDDEN_NEURONS,Hh);
			//发送ht,和Hh给本地主进程；
			MPI_Send(Hh,HIDDEN_NEURONS * HIDDEN_NEURONS,MPI_FLOAT,master_rank,0,MPI_COMM_WORLD);
			MPI_Send(Ht,HIDDEN_NEURONS * OUTPUT_NEURONS,MPI_FLOAT,master_rank,1,MPI_COMM_WORLD);
	}
	MPI_Finalize();
	return 0;
}
