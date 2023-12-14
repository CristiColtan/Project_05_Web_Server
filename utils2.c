#include "header.h"

void interpretPHP(const char *file_name)
{
    char command[256];

    snprintf(command, 256, "php %s > test.php", file_name);
    system(command);
}

void interpretJAVA(const char *file_name)
{
    char command[256];

    snprintf(command, 256, "node %s > testjava.txt", file_name);
    system(command);
}