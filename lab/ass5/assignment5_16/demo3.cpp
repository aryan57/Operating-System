#include "memlab.h"

#include <bits/stdc++.h>

using namespace std;

int main(){
    createMem(250*1024*1024);
    createVar("X1", 0);
    createVar("Y1", 0);
    createVar("X2", 0);
    createVar("Y2", 0);
    int cur = getPhyAddr();
    cout<<"Memory used when 4 individual bool variables are used  = "<<cur-4<<" bytes"<<endl;
    createArr("ar", 0, 4);
    cout<<"Memory used when array of bool with len 4 is used =  "<<getPhyAddr()-cur<<" bytes"<<endl;
    cout<<"So we can see that Array allocation is more efficient in this design"<<endl;
    return 0;
}