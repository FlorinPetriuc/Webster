#ifndef _HANDLERS_H_
#define _HANDLERS_H_

typedef int (*processor_t)(void *arg);

struct handler_prm_t
{
    int sockFD;
    int epoll_fd;

    unsigned char buffer_malloced;
    unsigned char *buffer;
    unsigned int buf_offset;
    unsigned int buf_len;

    unsigned char critical;
    unsigned char has_expiration;
    time_t expiration_date;

    struct http_request_t *request;

    processor_t processor;
};

#endif