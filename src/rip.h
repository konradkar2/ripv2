#ifndef RIP_H
#define RIP_H

typedef struct rip_context {
    int fd;
} rip_context;

int rip_begin(rip_context *rip_ctx);

#endif