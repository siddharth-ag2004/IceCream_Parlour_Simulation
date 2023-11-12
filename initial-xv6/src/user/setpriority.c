#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"

int main(int argc, char *argv[]) {
    if (argc != 3) 
    {
        printf("Usage: %s pid priority\n", argv[0]);
        exit(0);
    }

    int pid = atoi(argv[1]);
    int priority = atoi(argv[2]);

    if (set_priority(pid, priority) < 0) {
        printf("Failed to set priority for process %d\n", pid);
    }

    exit(0);
}