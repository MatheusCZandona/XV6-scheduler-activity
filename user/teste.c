#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
    int class = atoi(argv[1]); //classe por argumento

    int pid = fork(class);

    if (pid == 0) {
        while (1);  // filho roda
    }
    exit(0);
}
