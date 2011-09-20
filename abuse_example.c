#include <stdlib.h>

#include "abuse.h"

int main(int argc, char *argv[])
{
    struct abuse_operations aop;
    aop.size = 1024;

    return abuse_main(argc, argv, &aop);
}
