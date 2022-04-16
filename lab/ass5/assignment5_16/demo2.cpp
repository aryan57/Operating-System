#include "memlab.h"

#include <bits/stdc++.h>

using namespace std;

void fibonacci(int k){
    gc_init();

    createVar("product", 3);
    assignVar("product", 1);
    createArr("fib", 3, k+1);

    createVar("temp", 3);
    assignVar("temp", 0);
    assignArr("fib", 0, "temp");
    assignVar("temp", 1);
    assignArr("fib", 1, "temp");
    createVar("a", 3);
    createVar("b", 3);
    for(int i=2; i<=k; i++){
        assignVar("a", "fib", i-2);
        assignVar("b", "fib", i-1);
        assignArr("fib", i, AddInt("a", "b"));
        assignVar("a", "fib", i);
        assignVar("product", MultInt("product", "a"));
        printArr("fib", i);
    }

    assignVar("def", "product");
    gc_run(1, 1);
    return;
}

int main(){
    int K=10;
    printf("Enter the value of K ");
    scanf("%d", &K);
    createMem(4*(K+10));
    fibonacci(K);
    createVar("product", 3);
    assignVar("product", "def");
    printVar("product");
    return 0;
}