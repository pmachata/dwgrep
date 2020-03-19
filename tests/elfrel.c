#include <stdio.h>

int main(int argc, char *argv[]) {
    fprintf(stderr, "%s\n", argv[0]);
    puts(argv[0]);
}
