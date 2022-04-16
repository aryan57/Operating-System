#include "memlab.h"

#include <bits/stdc++.h>
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

using namespace std;

int logAddr=0;
int SZ;
int phyAddr=0;
int N_sym=1000;
char *mem;
unsigned int *phyMark;
// int h_map[1001];
// symbol symbolTable[1001];
int *h_map;
symbol *symbolTable;
int typeSz[4] = {1, 1, 3, 4};

//For garbage collection
pthread_mutex_t mem_lock;
stak st;



void gc_init(){
    st.push(-1);
}

void sweep(){
    while(st.peek() != -1){
        int idx = st.peek();
        st.pop();
        freeElem(idx);
    }
}

void compac(){
    // cout<<"called"<<endl;
    
    int n=SZ/4;
    // cout<<"called "<<n<<endl;
    int *b_link;
    b_link = (int *)malloc(sizeof(int)*n);
    // cout<<"called"<<endl;
    for(int i=0; i<1000; i++){
        if(h_map[i]!=-1){
            // cout<<"i "<<i<<endl;
            int idx = h_map[i];
            int phy = symbolTable[idx].phy_addr;
            b_link[phy/4] = idx;
        }
    } 
    int i=1, j=1;
    while(i<n && j<n){
        while(i<n && p_mark(i)==true){
            i++;
        }
        j=i;
        while(j<n && p_mark(j)==false){
            j++;
        }
        // cout<<"compac "<<i<<" "<<j<<endl;
        if(i<n && j<n){
            // cout<<"i "<<i<<"j "<<j<<endl;
            int idx = b_link[j];
            int type = symbolTable[idx].type;
            int len = symbolTable[idx].len;
            int fin = typeSz[type]*len;
            // cout<<"fin "<<fin<<endl;
            fin = ((fin+3)/4)*4;
            // cout<<"fin "<<fin<<endl;
            int l = fin/4;
            for(int k=0; k<l; k++){
                // phyMark[j+k]=false;
                set_pmark(j+k, false);
                // phyMark[i+k]=true;
                set_pmark(i+k, true);
                copyWord(4*(i+k), 4*(j+k));
            }
            symbolTable[idx].phy_addr = 4*i;
        }
    }
    phyAddr = 4*i;
    free(b_link);
}

void* collect(void *data){
    // cout<<"here"<<endl;
    pthread_mutex_lock(&mem_lock);
    // cout<<"locked"<<endl;
    shm flag = *((shm *)data);
    
    // cout<<"flag "<<flag.sweepFlag<<" "<<flag.compacFlag<<endl;
    if(flag.sweepFlag) sweep();
    // cout<<"here"<<endl;
    if(flag.compacFlag) compac();
    pthread_mutex_unlock(&mem_lock);
    // cout<<"unlocked"<<endl;
    pthread_exit(0);
}

void gc_run(int sweepFlag, int compacFlag){

	pthread_t tid;
	
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    shm data = {sweepFlag, compacFlag};
    // cout<<"flag "<<sweepFlag<<" "<<compacFlag<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Garbage collector called with sweepFlag = "<<sweepFlag<<" compacFlag = "<<compacFlag<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    pthread_create(&tid, &attr, collect, &data);
    usleep(1000);
    // pthread_join(tid, NULL);
}

bool p_mark(int idx){
    int i = idx/32;
    int j = idx%32;
    return (((phyMark[i])>>j)&1);
}

void set_pmark(int idx, bool val){
    int i = idx/32;
    int j = idx%32;
    
    unsigned int mask=0;

    if(val == false){
        for(int i=0; i<32; i++){
            if(i!=j){
                mask += (1<<i);
            }
        }
        phyMark[i] = phyMark[i]&mask;
    }else{
        for(int i=0; i<32; i++){
            if(i==j){
                mask += (1<<i);
            }
        }
        phyMark[i] = phyMark[i]|mask;
    }
}

int compute_hash(string const& s) {
    if(s == "def") return 0;
    const int p = 31;
    const int m = 1e9 + 9;
    long long hash_value = 0;
    long long p_pow = 1;
    for (char c : s) {
        int val = c;
        if(c<0) c+=255;
        hash_value = (hash_value + (c + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    // cout<<"hash of "<<s<<" "<<hash_value%N_sym<<endl;
    return hash_value%N_sym;
}

void createMem(int byts){
    st.top=0;
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    // pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK_NP);
    if(pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)!=0){
    	printf("Lock sharing Unsuccessful\n");
    	exit(1);
    }
    if(pthread_mutex_init(&(mem_lock), &mattr)!=0){
    	printf("Error : mutex_lock init() failed\n");
    	exit(1);
    }
    byts = ((byts+3)/4)*4;
    SZ = byts;
    // cout<<"Memory = "<<byts<<endl;
    int p_len = ((byts+31)/32);
    
    // cout<<"p_len = "<<p_len<<endl;
    // if(byts%4){
    //     printf("Error : Size must be multiple of 4\n");
    //     exit(1);
    // }
    // cout<<"byts "<<byts<<endl;
    int tot = byts+p_len+1000*(sizeof(symbol)+sizeof(int));
    char *totmem = (char *)malloc(tot);            //Total size taken

    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Total memory allocated in bytes  = "<<tot<<endl;
    cout<<"Overhead in bytes =  "<<tot-byts<<" i.e. "<<((tot*1.0-byts)/tot)*100<<" %"<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    // mem = (char *)malloc(sizeof(char)*(byts+10));
    //Distributing the memory
    mem = totmem;
    totmem+=(byts);
    // mem = new char[byts+10];
    // cout<<"mem "<<mem<<endl;
    // phyMark = (bool *)malloc(sizeof(bool)*(byts/4+10));
    h_map = (int *)totmem;
    totmem+=(1000*sizeof(int));
    phyMark = (unsigned int *)totmem;
    totmem+=(p_len);

    symbolTable = (symbol *)totmem;

    // cout<<"phyMark "<<phyMark<<endl;
    if(mem == NULL){
        printf("Error : Memory allocation failed\n");
        exit(1);
    }
    if(phyMark == NULL){
        printf("Error : Memory allocation failed\n");
        exit(1);
    }
    // cout<<"mem "<<mem<<endl;
    for(int i=0; i<1000; i++) h_map[i]=-1;
    // for(int i=0; i<20; i++) cout<<i<<" "<<h_map[i]<<endl;
    p_len = (p_len+3)/4;
    for(int i=0; i<p_len; i+=4) phyMark[i]=0;
    createVar("def", 3);
}

int firstFit(){
    for(int i=0; i<SZ; i+=4){
        if(p_mark(i/4)==false) return i;
    }
    return -1;
}

void createVar(string name, int type){
    if(mem == NULL){
        printf("Error : No memory allocated\n");
        exit(1);
    }
    if(logAddr>=1000){
        printf("Error : Symbol Table Overflow\n");
        exit(1);
    }
    pthread_mutex_lock(&mem_lock);
    int h_val = compute_hash(name);
    // for(int i=0; i<10; i++) cout<<i<<" "<<h_map[i]<<endl;
    if(h_map[h_val] != -1){
        // cout<<name<<" "<<h_val<<" "<<h_map[h_val]<<endl;
        printf("Error : Variable already declared\n");
        exit(1);
    }
    int phy = firstFit();
    if(phy == -1){
        printf("Error : Memory Overflow\n");
        exit(1);
    }
    st.push(h_val);
    // p_mark(phy/4) = true;
    set_pmark(phy/4, true);
    int idx = h_map[h_val] = logAddr;
    logAddr++;
    phyAddr = max(phyAddr, phy+4);
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Variable "<<name<<" created with logicalAddr = "<<idx<<" and phyAddr = "<<phy<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    symbolTable[idx].phy_addr = phy;
    symbolTable[idx].type = type;
    symbolTable[idx].mark = 0;
    symbolTable[idx].len = 1;
    pthread_mutex_unlock(&mem_lock);
}

void copyWord(int phy1, int phy2){
    for(int i=0; i<4; i++){
        mem[phy1+i] = mem[phy2+i];
    }
}

void copyWord(int phy1, int phy2, int len){
    for(int i=0; i<len; i++){
        mem[phy1+i] = mem[phy2+i];
    }
}

void assignVar(string n1, string n2){
    pthread_mutex_lock(&mem_lock);
    int idx1 = h_map[compute_hash(n1)];
    int idx2 = h_map[compute_hash(n2)];
    if(idx1==-1 || idx2==-1){
        printf("Error : Both variables must be declared first\n");
        exit(1);
    }
    if(symbolTable[idx1].type != symbolTable[idx2].type){
        printf("Error : Var type must be same in assign\n");
        exit(1);
    }
    int phy1 = symbolTable[idx1].phy_addr;
    int phy2 = symbolTable[idx2].phy_addr;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Variable "<<n1<<" assigned the value of "<<n2<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    copyWord(phy1, phy2);
    pthread_mutex_unlock(&mem_lock);
}

void assignVar(string n1, int x){
    pthread_mutex_lock(&mem_lock);
    int idx1 = h_map[compute_hash(n1)];
    if(idx1==-1){
        printf("Error : variable must be declared first\n");
        exit(1);
    }
    int phy1 = symbolTable[idx1].phy_addr;
    // cout<<idx1<<" "<<phy1<<endl;
    unsigned char *wd = (unsigned char*)(&x);

    for(int i=0; i<4; i++){
        // cout<<"i "<<i<<endl;
        *(mem+phy1+i) = *(wd+i);
    }
    // cout<<"unlock"<<endl;
    pthread_mutex_unlock(&mem_lock);
    // cout<<"unlock"<<endl;
}

void assignVar(string n1, string n2, int off){
    // cout<<"here"<<endl;
    pthread_mutex_lock(&mem_lock);
    // cout<<"here"<<endl;
    int idx1 = h_map[compute_hash(n1)];
    int idx2 = h_map[compute_hash(n2)];
    if(idx1==-1 || idx2==-1){
        printf("Error : Both variables must be declared first\n");
        exit(1);
    }
    if(symbolTable[idx1].type != symbolTable[idx2].type){
        printf("Error : Var type must be same in assign\n");
        exit(1);
    } 
    if(symbolTable[idx2].len<off){
        printf("Error : offset larger than actual len\n");
        exit(1);
    }
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Variable "<<n1<<" assigned the value of "<<n2<<"["<<off<<"]"<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    int phy1 = symbolTable[idx1].phy_addr;
    int type = symbolTable[idx1].type;
    int phy2 = symbolTable[idx2].phy_addr+off*typeSz[type];
    
    copyWord(phy1, phy2, typeSz[type]);
    pthread_mutex_unlock(&mem_lock);
    // cout<<"here"<<endl;
}

void printVar(string n1){
    pthread_mutex_lock(&mem_lock);
    int idx1 = h_map[compute_hash(n1)];
    if(idx1==-1){
        printf("Error : variable must be declared first\n");
        exit(1);
    }
    int phy1 = symbolTable[idx1].phy_addr;
    int type = symbolTable[idx1].type;
    if(type == 0){
        bool val = *((bool *)(mem+phy1));
        cout<<n1<<" = "<<val<<endl;
    }else if(type == 1){
        char val = *((char *)(mem+phy1));
        cout<<n1<<" = "<<val<<endl;
    }else{
        int val = *((int *)(mem+phy1));
        cout<<n1<<" = "<<val<<endl;
    }
    pthread_mutex_unlock(&mem_lock);
}

string AddInt(string n1, string n2){
    // cout<<"Add beg"<<endl;
    pthread_mutex_lock(&mem_lock);
    // cout<<"lock gained"<<endl;
    int idx1 = h_map[compute_hash(n1)];
    int idx2 = h_map[compute_hash(n2)];
    // cout<<idx1<<" "<<idx2<<endl;
    if(idx1==-1 || idx2==-1){
        printf("Error : Both variables must be declared first\n");
        exit(1);
    }
    if(symbolTable[idx1].type != symbolTable[idx2].type){
        printf("Error : Var type must be same in assign\n");
        exit(1);
    } 
    int phy1 = symbolTable[idx1].phy_addr;
    int phy2 = symbolTable[idx2].phy_addr;
    int val1 = *((int *)(mem+phy1));
    int val2 = *((int *)(mem+phy2));
    // cout<<"val1 "<<val1<<"  , val2 "<<val2<<endl;
    val1+=val2;
    pthread_mutex_unlock(&mem_lock);
    assignVar("def", val1);
    return "def";
    
    // cout<<"add end"<<endl;
}
string MultInt(string n1, string n2){
    pthread_mutex_lock(&mem_lock);
    int idx1 = h_map[compute_hash(n1)];
    int idx2 = h_map[compute_hash(n2)];
    if(idx1==-1 || idx2==-1){
        printf("Error : Both variables must be declared first\n");
        exit(1);
    }
    if(symbolTable[idx1].type != symbolTable[idx2].type){
        printf("Error : Var type must be same in assign\n");
        exit(1);
    } 
    int phy1 = symbolTable[idx1].phy_addr;
    int phy2 = symbolTable[idx2].phy_addr;
    int val1 = *((int *)(mem+phy1));
    int val2 = *((int *)(mem+phy2));
    val1*=val2;
    pthread_mutex_unlock(&mem_lock);
    assignVar("def", val1);
    return "def";
    
}

void createArr(string name, int type, int len){
    if(mem == NULL){
        printf("Error : No memory allocated\n");
        exit(1);
    }
    pthread_mutex_lock(&mem_lock);
    if(logAddr>=1000){
        printf("Error : Symbol Table Overflow\n");
        exit(1);
    }
    int h_val = compute_hash(name);
    if(h_map[h_val] != -1){
        // cout<<"name "<<name<<endl;
        printf("Error : Array already declared\n");
        exit(1);
    }
    int phy = phyAddr;
    int fin = typeSz[type]*len;
    // cout<<"fin "<<fin<<endl;
    fin = ((fin+3)/4)*4;
    // cout<<"fin "<<fin<<endl;
    int l = fin/4;
    // cout<<SZ<<" "<<phy<<" "<<l<<endl;
    if((SZ-phy)/4 < l){
        pthread_mutex_unlock(&mem_lock);
        gc_run(0, 1);
        pthread_mutex_lock(&mem_lock);
        if((SZ-phyAddr)/4 < l){
            printf("Error : Memory Overflow\n");
            exit(1);
        }
    }
    st.push(h_val);
    for(int i=0; i<l; i++){
        // phyMark[phy/4+i]=true;
        set_pmark(phy/4+i, true);
    }
    
    int idx = h_map[h_val] = logAddr;
    logAddr++;
    phyAddr += l*4;
    // cout<<phyAddr<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Array "<<name<<" created with logicalAddr = "<<idx<<" and phyAddr = "<<phy<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    symbolTable[idx].phy_addr = phy;
    symbolTable[idx].type = type;
    symbolTable[idx].mark = 0;
    symbolTable[idx].len = len;
    pthread_mutex_unlock(&mem_lock);
}
void assignArr(string name, int pos, string var){
    // cout<<"here beg"<<endl;
    pthread_mutex_lock(&mem_lock);
    int idx1 = h_map[compute_hash(name)];
    int idx2 = h_map[compute_hash(var)];
    if(idx1==-1 || idx2==-1){
        printf("Error : Both variables must be declared first\n");
        exit(1);
    }
    if(symbolTable[idx1].type != symbolTable[idx2].type){
        printf("Error : Var type must be same in assign\n");
        exit(1);
    } 
    // cout<<"pos "<<pos<<" len "<<symbolTable[idx1].len<<endl;
    if(pos>=symbolTable[idx1].len){
        printf("Error : Offset larger than actual length\n");
        exit(1);
    }
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Variable "<<name<<"["<<pos<<"]"<<" assigned the value of "<<var<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    int phy1 = symbolTable[idx1].phy_addr;
    int phy2 = symbolTable[idx2].phy_addr;
    int type = symbolTable[idx1].type;
    copyWord(phy1+pos*typeSz[type], phy2, typeSz[symbolTable[idx1].type]);
    pthread_mutex_unlock(&mem_lock);
    // cout<<"here end"<<endl;
}

// void assignArr(string name, int pos, int x){
//     int idx1 = h_map[compute_hash(name)];
//     if(idx1==-1){
//         printf("Error : variable must be declared first\n");
//         exit(1);
//     }
//     if(pos>=symbolTable[idx1].len){
//         printf("Error : Offset larger than actual length\n");
//         exit(1);
//     }
//     int phy1 = symbolTable[idx1].phy_addr+4*pos;
//     unsigned char *wd = (unsigned char*)(&x);
//     for(int i=0; i<4; i++){
//         *(mem+phy1+i) = *(wd+i);
//     }
// }

void printArr(string n1, int idx){
    pthread_mutex_lock(&mem_lock);
    int idx1 = h_map[compute_hash(n1)];
    if(idx1==-1){
        printf("Error : variable must be declared first\n");
        exit(1);
    }
    int type = symbolTable[idx1].type;
    int phy1 = symbolTable[idx1].phy_addr+idx*(typeSz[type]);
    
    if(type == 0){
        bool val = *((bool *)(mem+phy1));
        cout<<n1<<"["<<idx<<"]"<<" = "<<val<<endl;
    }else if(type == 1){
        char val = *((char *)(mem+phy1));
        cout<<n1<<"["<<idx<<"]"<<" = "<<val<<endl;
    }else if(type == 2){
        m_int v;
        v = *((m_int *)(mem+phy1));
        int val = *((int *)(v.ar));
        cout<<n1<<"["<<idx<<"]"<<" = "<<val<<endl;
    }else{
        int val = *((int *)(mem+phy1));
        cout<<n1<<"["<<idx<<"]"<<" = "<<val<<endl;
    }
    pthread_mutex_unlock(&mem_lock);
}

// void freeElem(string name){
//     int idx1 = h_map[compute_hash(name)];
//     int phy1 = symbolTable[idx1].phy_addr;
//     h_map[compute_hash(name)] = -1;
//     symbolTable[idx1].mark=1; // marked for deletion in logical space
// }

void freeElem(int h_val){
    int idx = h_map[h_val];
    int phy = symbolTable[idx].phy_addr;
    int type = symbolTable[idx].type;
    int len = symbolTable[idx].len;

    int fin = typeSz[type]*len;
    // cout<<"fin "<<fin<<endl;
    fin = ((fin+3)/4)*4;
    // cout<<"fin "<<fin<<endl;
    int l = fin/4;
    // cout<<SZ<<" "<<phy<<" "<<l<<endl;
    
    // cout<<"Freeing Phy Addrs"<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<< Message >>>>>>>>>>>>>>>>>"<<endl;
    cout<<"Freeing words starting at "<<phy<<" len = "<<l<<endl;
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<         >>>>>>>>>>>>>>>>>"<<endl<<endl;
    for(int i=0; i<l; i++){
        // cout<<4*(phy/4+i)<<endl;
        // phyMark[phy/4+i]=false;
        set_pmark(phy/4+i, false);
    }
    h_map[h_val]=-1;
}

int getType(string name){
    int idx1 = h_map[compute_hash(name)];
    int type = symbolTable[idx1].type;

    return type;
}

int getRandom(int type){
    int sz = typeSz[type];
    if(type == 0){
        return rand()%2;
    }else{
        char ar[4] = {0, 0, 0, 0};
        for(int i=0; i<sz; i++){
            ar[i] = rand()%255;
        }
        int val = *((int *)(ar));
        return val;
    }
}

int getPhyAddr(){
    return phyAddr;
}
