#ifndef ALG_H__
#define ALG_H__

#define MAX 20
 
struct child{
	int id;
	struct child *next;
};

struct Dtree{
	int id;
	int parent;
	struct child *next;
}; 
struct Dtree dt[MAX];
void create_dtree(int size,int d);
int MPIN_Reduce(float *sendbuf,float *recvbuf,int count,int cur_node_id,int cur_rank,int tag);

#endif
