#ifndef _HANDLERS_H_
#define _HANDLERS_H_

typedef void (*processor_t)(void *arg);

struct handler_prm_t
{
    int sockFD;
    int epoll_fd;

    unsigned char *buffer;
    unsigned int buf_offset;
    unsigned int buf_len;

    unsigned char critical;

    processor_t processor;
};

#endif