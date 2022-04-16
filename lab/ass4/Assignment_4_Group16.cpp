#include "stdio.h"
#include "bits/stdc++.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "fcntl.h"
#include "ctype.h"
#include "signal.h"
#include "sys/mman.h"
#include "time.h"
#include "pthread.h"
#include "stdbool.h" 

using namespace std;

#define child_lim   20
#define T_lim       50000
#define ID_lim      100000000

int NP, NC;         //number of producer threads and consumer threads
int wr=0;

typedef struct node
{
	int id;
	int t_ms;                 //completion time in miliseconds
	int child[child_lim];
	int child_cnt;
	pthread_mutex_t lock;
	int parent_id;
	int proc_stat;            //-1 -> done, 0 -> not processed, 1 -> under process  
}Node;

typedef struct _SHM{
	Node T[T_lim];
	int cnt;
	pthread_mutex_t lock;
}SHM;

SHM * data;

void mem_init(){

	for(int i=0; i<T_lim; i++){
	    pthread_mutexattr_t mattr;
	    pthread_mutexattr_init(&mattr);
	    // pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK_NP);
	    if(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)!=0){
	    	printf("Lock sharing Unsuccessful\n");
	    	exit(1);
	    }
	    if(pthread_mutex_init(&((data->T[i]).lock), &mattr)!=0){
	    	printf("Error : mutex_lock init() failed\n");
	    	exit(1);
	    }
	}

	srand(clock());
	int N = rand()%201+300;
	// int N = rand()%10+5;
	printf("N = %d\n", N);
	int cur_i=1;
	(data->T[0]).parent_id = -1;

	queue<int> q;
	q.push(0);
	while(!q.empty()){
		int i = q.front();

		q.pop();
		(data->T[i]).id = i;
		(data->T[i]).t_ms = rand()%250+1;
		(data->T[i]).child_cnt = 0;
		memset((data->T[i]).child, -1, sizeof((data->T[i]).child));
		// printf("New Node, i = %d, completion time = %d\n", i, (data->T[i]).t_ms);
		// printf("cur_i = %d\n", cur_i);
		if(cur_i<N){
			// printf("children list ->  ");
			int j = rand()%child_lim+1;
			(data->T[i]).child_cnt = j;

			for(int k=0; k<j; k++){
				(data->T[k+cur_i]).parent_id = i;
				q.push(k+cur_i);
				(data->T[i]).child[k] = k+cur_i;
				// printf("%d ", (data->T[i]).child[k]);
			}
			// printf("\n\n");
			cur_i+=j;
		}

		(data->T[i]).proc_stat = 0;
	}
	data->cnt = cur_i;
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    // pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK_NP);
    if(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)!=0){
    	printf("Lock sharing Unsuccessful\n");
    	exit(1);
    }
    if(pthread_mutex_init(&(data->lock), &mattr)!=0){
    	printf("Error : mutex_lock init() failed\n");
    	exit(1);
    }
}

void* produce_thread(void *param){
	srand(clock()+wr);
	wr++;
	int runtime = rand()%11+10;
	printf("Producer Runtime = %d\n", runtime);
	time_t begin;
    time(&begin);
    
    int exit_flag=0;
    
    while(1){
    	int wt = rand()%301+200;
    	// printf("Producer Sleeping for %d ms\n", wt);
    	usleep(wt*1000);

    	pthread_mutex_lock(&(data->lock));
    	if(data->cnt >= T_lim) exit_flag=1;
    	int i = rand()%(data->cnt);
    	pthread_mutex_unlock(&(data->lock));
    	if(exit_flag) break;


    	pthread_mutex_lock(&((data->T[i]).lock));
    	if(data->T[i].proc_stat == 0 && data->T[i].child_cnt < child_lim){ //not scheduled
	    	data->T[i].child_cnt++;
	    	pthread_mutex_lock(&(data->lock));
	    	int j = data->cnt;
	    	pthread_mutex_lock(&((data->T[j]).lock));
	    	(data->cnt)++;
	    	pthread_mutex_unlock(&(data->lock));
	    	for(int k=0; k<child_lim; k++){
	    		if(data->T[i].child[k] == -1){
	    			data->T[i].child[k] = j;
	    			break;
	    		}
	    	}
			(data->T[j]).id = j;
			(data->T[j]).t_ms = rand()%250+1;
			(data->T[j]).child_cnt = 0;
			memset((data->T[j]).child, -1, sizeof((data->T[j]).child));
			(data->T[j]).proc_stat = 0;
			(data->T[j]).parent_id = i;
			printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<< NEW  JOB >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
			printf("Job id = %d, parent\'s job id = %d\n", j, i);
			printf("Execution Time = %d ms\n", (data->T[j]).t_ms);
			printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<< XXX  XXX >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n\n");
			pthread_mutex_unlock(&((data->T[j]).lock));
    	}
    	pthread_mutex_unlock(&((data->T[i]).lock));
    	

		time_t end;
		time(&end);
		if(difftime(end, begin)>runtime) break;
    }
    printf("<<<<<<< Producer Exiting >>>>>>>>>\n");
	pthread_exit(0);
}

void producer(){
	pthread_t tid[NP];
	for(int i=0; i<NP; i++){
		pthread_attr_t attr;
		pthread_attr_init (&attr);
		pthread_create(&tid[i], &attr, produce_thread, NULL);
	}
	for(int i=0; i<NP; i++){
		pthread_join(tid[i], NULL);
	}

}

int dfs(int seed){
	stack<int> st;
	st.push(0);
	while(!st.empty()){
		int i = st.top();
		// printf("searching i = %d\n", i);
		st.pop();
		pthread_mutex_lock(&((data->T[i]).lock));
		if(data->T[i].child_cnt == 0 && data->T[i].proc_stat == 0){
			// printf("Found i = %d\n", i);
			return i;
		}
		int arr[child_lim];
		for(int j=0; j<child_lim; j++){
			arr[j] = data->T[i].child[j];
		}
		shuffle(arr, arr + child_lim, default_random_engine(seed));
		for(int j=0; j<child_lim; j++){
			if(arr[j]!=-1) st.push(arr[j]);
		}
		pthread_mutex_unlock(&((data->T[i]).lock));

	}
	return -1;
}

void* consume_thread(void *param){

	int seed = wr;
	wr++;
    srand(clock()+seed);
    
    int i=0;
    while(i>=0){
    	i = dfs(seed);
    	if(i<0) break;
    	int wt = data->T[i].t_ms;
    	int p_id = data->T[i].parent_id;

		printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<< JOB STARTED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("Job id = %d, parent\'s job id = %d\n", i, p_id);
		printf("Execution Time = %d ms\n", (data->T[i]).t_ms);
		printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<< XXX  XXXXXXX >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n\n");
		data->T[i].proc_stat = 1;
    	usleep(wt*1000);
    	data->T[i].proc_stat = -1;
		printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<< JOB COMPLETED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("Job id = %d, parent\'s job id = %d\n", i, p_id);
		printf("Execution Time = %d ms\n", (data->T[i]).t_ms);
		printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<< XXX  XXXXXXX >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n\n");
    	pthread_mutex_lock(&((data->T[p_id]).lock));
    	for(int j=0; j<child_lim; j++){
    		if(data->T[p_id].child[j] == i){
    			data->T[p_id].child[j] = -1;
    			data->T[p_id].child_cnt--;
    			break;
    		}
    	}
    	pthread_mutex_unlock(&((data->T[p_id]).lock));
    	pthread_mutex_unlock(&((data->T[i]).lock));
	}
	printf("<<<<<<< Consumer Exiting >>>>>>>>>\n");
	pthread_exit(0);
}

void consumer(){
	pthread_t tid[NC];
	for(int i=0; i<NC; i++){
		pthread_attr_t attr;
		pthread_attr_init (&attr);
		pthread_create(&tid[i], NULL, consume_thread, NULL);
	}
	for(int i=0; i<NC; i++){
		pthread_join(tid[i], NULL);
	}
}

int main(){
	printf("Enter the number of producer threads\n");
	scanf("%d", &NP);
	printf("Enter the number of consumer threads\n");
	scanf("%d", &NC);

	time_t begin;
    time(&begin);
	data = (SHM *)mmap(NULL, sizeof(SHM), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	mem_init();

	pid_t pid = fork();
	if(pid==0){  //consumer process forked
		consumer();
		exit(0);
	}else{
		producer();
		int status;
		waitpid(pid, &status, 0);
	}

	time_t end;
	time(&end);
	printf("Execution time %f s\n", difftime(end, begin));

	return 0;
}