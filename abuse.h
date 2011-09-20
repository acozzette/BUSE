#ifndef ABUSE_H_INCLUDED
#define ABUSE_H_INCLUDED

struct abuse_operations {
    void (*read)();
    void (*write)();
    void (*disc)();
    void (*flush)();
    void (*trim)();
};

int abuse_main();

#endif /* ABUSE_H_INCLUDED */
