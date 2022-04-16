#include "stdio.h"
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

typedef long long int lli;

#define N 100
#define q_size 10

int job_lim;

/*
	queue of structs
	each struct has one matrix and all status
*/
//pthread_mutex_lock(&(data->lock));
//pthread_mutex_unlock(&(data->lock));

typedef struct _mat{
	int prod_num;
	int mat_id;
	lli grid[N][N];
	 
	/*
		0 -> no addition, 1 -> one addition, 2 -> two addition(completed);
	*/
	int creation_status[2][2];
	int mult_status[8];

}mat;

typedef struct _job_q{
	int front, rear, cur_size;
	mat job[q_size];
}queue;

int isFull(queue *q){
	return (q->cur_size)>=q_size;
}
int enqueue(queue *q, mat a){
	if((q->cur_size)>=q_size){
		return 0;
	}
	(q->cur_size)++;
	q->job[q->rear] = a;
	q->rear = (q->rear+1)%q_size;
	return 1;
}
int dequeue(queue *q){
	if(q->cur_size == 0) return 0;
	q->front = (q->front+1)%q_size;
	(q->cur_size)--;
	return 1;
}

typedef struct _SHM{
	pthread_mutex_t lock;
	queue job_q;
	int job_created;
}SHM;

SHM * data;

void mem_init(){
	data->job_created = 0;
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
   	(data->job_q).front = 0;
   	(data->job_q).rear = 0;
   	(data->job_q).cur_size = 0; 
}

int creation_sum(queue job_q){
	if(job_q.cur_size==0) return 1;
	int i = (job_q.rear-1+q_size)%q_size;

	mat temp = job_q.job[i];
	int sum=0;
	for(int i=0; i<2; i++){
		for(int j=0; j<2; j++){
			sum += temp.creation_status[i][j];
		}
	}
	return sum;
}

bool no_create(queue job_q){
	if(job_q.cur_size==0) return 1;
	int i = (job_q.rear-1+q_size)%q_size;

	mat temp = job_q.job[i];
	int sum=0;
	for(int i=0; i<2; i++){
		for(int j=0; j<2; j++){
			sum += temp.creation_status[i][j];
		}
	}
	bool flag = (sum==8);
	flag &= (job_q.cur_size < q_size);
	return flag;
}

int mult_status(queue job_q){
	// returns block number to access, -1 if none available
	if(job_q.cur_size<2) return -1;
	for(int i=0; i<8; i++){
		if(job_q.job[job_q.front].mult_status[i] == 0){
			return i;
		}
	} 
	return -1;
}

void producer(int id){
	while((data->job_created)<job_lim){
		mat n_job;
		n_job.prod_num = id;
		srand(id+clock());
		n_job.mat_id = rand()%(100000)+1;
		for(int i=0; i<N; i++){
			for(int j=0; j<N; j++){
				n_job.grid[i][j] = rand()%(19)-9; 
			}
		}
		for(int i=0; i<2; i++){
			for(int j=0; j<2; j++){
				n_job.creation_status[i][j]=2;
			}
		}
		for(int i=0; i<8; i++){
			n_job.mult_status[i]=0;
		}

		double st = rand()%3000;
		st/=1000.0;
		// printf("Producer %d will sleep for %lf s\n", id, st);
		st = st*1000000;
		usleep(st);

		pthread_mutex_lock(&(data->lock));

		// printf("\n\ntime : %ld\n\n", clock());
		if(no_create(data->job_q) && (data->job_created)<job_lim){
			enqueue(&(data->job_q), n_job);
			(data->job_created)++;
			printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
			printf("Job Generated : \n");
			printf("Producer ID : %d ,  PID : %d\n", id, getpid());
			printf("Matrix id : %d\n", n_job.mat_id);
			// for(int i=0; i<N; i++){
			// 	for(int j=0; j<N; j++){
			// 		printf("%lld  ", n_job.grid[i][j]);
			// 	}
			// 	printf("\n");
			// }
			printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");
		}

		pthread_mutex_unlock(&(data->lock));
	}
	// printf("job_count %d\n", data->job_created);
}

void worker(int id){
	while(data->job_created<job_lim || (data->job_q).cur_size != 1){
		srand(id+clock());
		double st = rand()%3000;
		st/=1000.0;
		// printf("Worker %d will sleep for %lf s\n", id, st);
		st = st*1000000;
		usleep(st);

		pthread_mutex_lock(&(data->lock));
		lli A[N][N], B[N][N], C[N][N];
		int b_id = mult_status(data->job_q);
		mat a,b;
		if(b_id != -1){
			queue job_q = data->job_q;
			int ii = (job_q.front+1)%q_size;
			a = job_q.job[job_q.front];
			b = job_q.job[ii];
			(data->job_q).job[job_q.front].mult_status[b_id]=(data->job_q).job[ii].mult_status[b_id]=1;
			// A = job_q.job[job_q.front].grid;
			// B = job_q.job[i].grid;
			for(int i=0; i<N; i++){
				for(int j=0; j<N; j++){
					A[i][j]=job_q.job[job_q.front].grid[i][j];
					B[i][j]=job_q.job[ii].grid[i][j];
				}
			}
			printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
			printf("Worker %d fetched blocks with following details : \n", id);
			printf("Matrix 1\n");
			printf("Producer ID : %d Matrix id : %d\n", a.prod_num, a.mat_id);
			printf("Matrix 2\n");
			printf("Producer ID : %d Matrix id : %d\n", b.prod_num, b.mat_id);
			printf("Block Num : %d\n", b_id);
			printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");
		}
		
		pthread_mutex_unlock(&(data->lock));


		if(b_id!=-1){
			memset(C, 0, sizeof(C));

			int i_off=0;
			int j_off=0;
			int k_off=0;

			if(b_id%2) k_off+=(N/2);
			if(b_id>3) i_off+=(N/2);
			if(b_id==2 || b_id==3 || b_id==6 || b_id==7) j_off+=N/2;

			for(int i=0+i_off; i<N/2+i_off; i++){
				for(int j=0+j_off; j<N/2+j_off; j++){
					lli temp=0;
					for(int k=0+k_off; k<N/2+k_off; k++){
						temp+=A[i][k]*B[k][j];
					}
					C[i][j]=temp;
				}
			}

			pthread_mutex_lock(&(data->lock));
			
			b_id = mult_status(data->job_q);
			if(b_id==-1){
				dequeue(&(data->job_q));
				dequeue(&(data->job_q));
				printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
				printf("Worker %d removed follwing matrices : \n", id);
				printf("Matrix 1\n");
				printf("Producer ID : %d Matrix id : %d\n", a.prod_num, a.mat_id);
				printf("Matrix 2\n");
				printf("Producer ID : %d Matrix id : %d\n", b.prod_num, b.mat_id);
				printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

			}
			if(creation_sum(data->job_q)>=8 && (data->job_q).cur_size > 1){
				if((data->job_q).cur_size>=q_size){
					printf("\n\n\n<<<<<<<<DeadLock Reached : Queue Full>>>>>>>>\n\n");
					exit(0);
				}
				mat n_job;
				n_job.prod_num = id;
				srand(id+clock());
				n_job.mat_id = rand()%(100000)+1;
				// n_job.grid = C;
				for(int i=0; i<N; i++){
					for(int j=0; j<N; j++){
						n_job.grid[i][j] = C[i][j];
					}
				}
				for(int i=0; i<2; i++){
					for(int j=0; j<2; j++){
						n_job.creation_status[i][j]=0;
					}
				}
				for(int i=0; i<8; i++){
					n_job.mult_status[i]=0;
				}
				n_job.creation_status[(i_off)/(N/2)][(j_off)/(N/2)]=1;
				enqueue(&(data->job_q), n_job);
				printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
				printf("Intermediate Job Generated from %d and %d \n", a.mat_id, b.mat_id);
				printf("Producer ID : %d ,  PID : %d\n", id, getpid());
				printf("Matrix id : %d\n", n_job.mat_id);
				printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");
				// for(int i=0; i<N; i++){
				// 	for(int j=0; j<N; j++){
				// 		printf("%lld  ", n_job.grid[i][j]);
				// 	}
				// 	printf("\n");
				// }
			}else if(creation_sum(data->job_q) < 8){
				int idx = ((data->job_q).rear-1+q_size)%q_size;
				for(int i=0; i<N; i++){
					for(int j=0; j<N; j++){
						(data->job_q).job[idx].grid[i][j] += C[i][j];
					}
				}
				(data->job_q).job[idx].creation_status[(i_off)/(N/2)][(j_off)/(N/2)]+=1;
				printf("\n\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
				printf("Worker %d added to follwing block : \n", id);
				printf("%d, %d\n", (i_off)/(N/2), (j_off)/(N/2));
				printf("of Matrix with id : %d\n", (data->job_q).job[idx].mat_id);
				printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");
			}
			pthread_mutex_unlock(&(data->lock));
		}
	}

}

int NP=0, NW=0;
pid_t child_pid[100];

void SIGINT_handler(int sig){
	for(int i=0; i<NP+NW; i++){
		kill(child_pid[i], SIGINT);
	}
	exit(0);
}

int main(){	
	signal(SIGINT, SIGINT_handler);
	int NP, NW;
	printf("Enter the nos of producers and workers (space separated) : \n");
	scanf("%d %d", &NP, &NW);
	printf("Enter the nos of matrices to multiply : \n");
	scanf("%d", &job_lim);
	time_t begin;
    time(&begin);
	data = (SHM *)mmap(NULL, sizeof(SHM), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	mem_init();

	for(int i=0; i<NP; i++){
		pid_t pid = fork();
		if(pid == 0){
			producer(i+1);
			exit(EXIT_SUCCESS);
		}else{
			child_pid[i]=pid;
		}
	}
	for(int i=0; i<NW; i++){
		pid_t pid = fork();
		if(pid == 0){
			worker(i+1);
			exit(EXIT_SUCCESS);
		}else{
			child_pid[NP+i]=pid;
		}
	}

	while(data->job_created<job_lim || (data->job_q).cur_size != 1){
		usleep(200000);
	}

	// while(data->job_created<job_lim){
	// 	usleep(200000);
	// }
	sleep(1);

	printf("\n\n<<<<<<<Final Answer>>>>>>\n\n");
	lli ans=0;

	queue job_q = data->job_q;
	mat temp = job_q.job[job_q.front];
	for(int i=0; i<N; i++){
		ans+=temp.grid[i][i];
	}
	printf("The sum of principal Diagonal is : %lld\n", ans);
	// for(int i=job_q.front; i<job_q.rear; i++){
	// 	mat temp = job_q.job[i];
	// 	printf("Matrix id : %d\n", temp.mat_id);
	// 	for(int i=0; i<N; i++){
	// 		for(int j=0; j<N; j++){
	// 			printf("%lld  ", temp.grid[i][j]);
	// 		}
	// 		printf("\n");
	// 	}
	// }

	time_t end;
	time(&end);
	printf("Execution time %f s\n", difftime(end, begin));

	return 0;
}