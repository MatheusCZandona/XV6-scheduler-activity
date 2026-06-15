#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) { //(trabalho)
    
    if(strlen(argv[1]) == 0 || argc == 1){
        printf("using default: 200\n");
        int pid = fork(200);
        
        if (pid == 0) {
            while (1);  // filho
        }
        exit(0);
    }

    char* num = argv[1];
    int tickets = 0;

        for(int i = 0; i < strlen(argv[1]); i++){
                if(num[i] >= '0' && num[i] <= '9'){
                    tickets *= 10;
                    tickets += argv[1][i] - '0';
                } else {
                    printf("invalid argument\n");
                    exit(0);
                }
        }

    tickets %= 5000;
    tickets++;
    int pid = fork(tickets);

    if (pid == 0) {
        while (1);  // filho
    }
    exit(0);
}
