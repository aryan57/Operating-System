#include "memlab.h"

#include <bits/stdc++.h>

using namespace std;

void func(string x, string y){
    gc_init();
    createArr("ar", getType(x), 50000);
    for(int i=0; i<50000; i++){
        assignVar(x, getRandom(getType(x)));
        assignArr("ar", i, x);
        // printArr("ar", i);
    }
    gc_run(1, 1);
}


int main(){
    createMem(250*1024*1024);
    createVar("X1", 0);
    createVar("Y1", 0);
    func("X1", "Y1");

    createVar("X2", 1);
    createVar("Y2", 1);
    func("X2", "Y2");

    createVar("X3", 2);
    createVar("Y3", 2);
    func("X3", "Y3");

    createVar("X4", 3);
    createVar("Y4", 3);
    func("X4", "Y4");

    func("X1", "Y1");
    func("X2", "Y2");
    func("X3", "Y3");
    func("X4", "Y4");
    func("X1", "Y1");
    func("X2", "Y2");
    return 0;
}