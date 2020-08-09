#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    if (daemon(1, 1)) {
        perror("daemon");
        exit(1);
    }
    for (;;) {}
}

