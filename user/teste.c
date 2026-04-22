#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    
    if(strlen(argv[1]) == 0){
        printf("invalid argument\n");
        exit(0);
    }
    char* num = argv[1];
    int class = 0;

    if(num[0] == '-'){
        if((strlen(argv[1]) >= 2) && num[1] >= '0' && num[1] <= '9'){
            for(int i = 1; i < strlen(argv[1]); i++){
                class *= 10;
                class += argv[1][i] - '0';
            }
        } else {
            printf("invalid argument\n");
            exit(0);
        }
    } else {
        if(num[0] >= '0' && num[0] <= '9'){
            for(int i = 0; i < strlen(argv[1]); i++){
                class *= 10;
                class += argv[1][i] - '0';
            }
        } else {
            printf("invalid argument\n");
            exit(0);
        }
    }

    class %= 4;
    
    int pid = fork(class);

    if (pid == 0) {
        while (1);  // filho
    }
    exit(0);
}
