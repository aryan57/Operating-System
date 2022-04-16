#ifndef __MEMLAB_H
#define __MEMLAB_H
#include<bits/stdc++.h>
#include<pthread.h>
using namespace std;

struct symbol{
    int phy_addr;  //word position in block memory, in case of array start position
    int type;      //0 -> bool, 1 ->char, 2 -> medium_int, 3 -> int
    int mark;      //whether the variable is marked for deletion(mark phase)
    int len;       //greater than 1 signifies array
};

struct m_int{
    char ar[3];
};

struct stak{
    int ar[1001];
    int top;
    int peek(){
        if(top <= 0){
            // cout<<"peek "<<-1<<endl;
            return -1;
        }
        else{
            // cout<<"peek "<<ar[top-1]<<endl;
            return ar[top-1];
        }
    }

    void push(int val){
        if(top >= 1000){
            printf("Error : Var Stack Overflow\n");
        }
        ar[top] = val;
        // cout<<"push "<<val<<endl;
        top++; 
    }

    void pop(){
        if(top>0){
            // cout<<"pop "<<ar[top-1]<<endl;
            top--;
        }
    }
};

struct shm{
    int sweepFlag;
    int compacFlag;
};

int compute_hash(string const& s);
void createMem(int byts);
int firstFit();
void printVar(string n1);
void printArr(string n1, int idx);

void createVar(string name, int type);
void assignVar(string n1, string n2);
void assignVar(string n1, int x);
void assignVar(string n1, string n2, int off);

void createArr(string name, int type, int len);
void assignArr(string name, int pos, string var);
// void assignArr(string name, int pos, int x);

string AddInt(string n1, string n2);
string MultInt(string n1, string n2);

void freeElem(int h_val);
void gc_init();
void gc_run(int sweepFlag, int compacFlag);
void copyWord(int phy1, int phy2);
void copyWord(int phy1, int phy2, int len);
int getType(string name);
int getRandom(int type);
bool p_mark(int idx);
void set_pmark(int idx, bool val);

double get_gtime();
int getPhyAddr();

#endif


/*TODO
-- add locks
-- Garbage Collection
   - keep a global stack of idx to remember variables in scope
   - when put -1 when new scope starts in the stack
   - when the scope ends(function returns) pop untill -1 ans mark those (mark phase) for deletion in symbol table
   - finally gc_run uses free_elem to mark them as deleted in phyMark
   - compac() moves the memory blocks to remove holes 
*/