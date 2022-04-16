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

typedef struct _ProcessData{
	double *A;
	double *B;
	double *C;
	int i, j, k, len;
}ProcessData;

void *mult(void *arg){
	ProcessData *data = (ProcessData*)arg;
	double val=0;
	int r1 = data->i;
	int c1 = data->j;
	int c = data->len;
	int k = data->k;
	for(int i=0; i<c; i++){
		val += (data->A[c*r1+i])*(data->B[i*k+c1]);
	}
	data->C[r1*k+c1] = val;
	// printf("calculated for %d, %d =  %lf\n", r1, c1, val);
}

int main(){
	int r1,c1, r2,c2;
	// printf("Enter the number of rows and columns for A (space separated)\n");
	scanf("%d %d", &r1, &c1);
	double a[r1][c1];
	// printf("Enter the elements of array A\n");
	for(int i=0; i<r1; i++){
		for(int j=0; j<c1; j++){
			scanf("%lf", &a[i][j]);
		}
	}

	// printf("Enter the number of rows and columns for B (space separated)\n");
	scanf("%d %d", &r2, &c2);
	if(c1!=r2){
		printf("Error : c1 != r2\n");
		exit(0);
	}
	double b[r2][c2];
	// printf("Enter the elements of array B\n");
	for(int i=0; i<r2; i++){
		for(int j=0; j<c2; j++){
			scanf("%lf", &b[i][j]);
		}
	}


	int ar_sz = (r1*c1+r2*c2+r1*c2)*sizeof(double);
	
	double *grid = (double *)mmap(NULL, ar_sz+10000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(grid == MAP_FAILED){
		printf("Error : MAP_FAILED\n");
		exit(0);
	} 

	double *A = grid;
	double *B = grid + r1*c1;
	double *C = grid + r1*c1+r2*c2; 
	double x;

	for(int i=0; i<r1; i++){
		for(int j=0; j<c1; j++){
			*(A+i*c1+j) = a[i][j];
		}
	}

	for(int i=0; i<r2; i++){
		for(int j=0; j<c2; j++){
			*(B+i*c2+j) = b[i][j];
		}
	}


	// for(int i=0; i<r1; i++){
	// 	for(int j=0; j<c1; j++){
	// 		printf("%lf ", *(A+i*c1+j));
	// 	}
	// 	printf("\n");
	// }

	// for(int i=0; i<r2; i++){
	// 	for(int j=0; j<c2; j++){
	// 		printf("%lf ", *(B+i*c2+j));
	// 	}
	// 	printf("\n");
	// }

	ProcessData p_data[r1][c2];
	pid_t child_pid[r1][c2];
	for(int i=0; i<r1; i++){
		for(int j=0; j<c2; j++){
			p_data[i][j].A = A;
			p_data[i][j].B = B;
			p_data[i][j].C = C;
			p_data[i][j].i = i;
			p_data[i][j].j = j;
			p_data[i][j].k = c2;
			p_data[i][j].len = c1;
			pid_t pid = fork();
			if(pid == 0){
				mult(&p_data[i][j]);
				exit(0);
			}else{
				child_pid[i][j] = pid;
			}
		}
	}
	int status;
	for(int i=0; i<r1; i++){
		for(int j=0; j<c2; j++){
			waitpid(child_pid[i][j], &status, 0);
		};
	}

	for(int i=0; i<r1; i++){
		for(int j=0; j<c2; j++){
			printf("%lf ", *(C+i*c2+j));
		}
		printf("\n");
	}

}