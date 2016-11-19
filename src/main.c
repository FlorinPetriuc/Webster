#include "main.h"

int main(int argc, char **argv)
{
    int ret;
    int epoll_fd;
    int srvFD;

    struct handler_prm_t *acc_str;

    logInit(NULL);
    logWrite(LOG_TYPE_INFO, "Starting up webster v%d", 1, WEBSTER_VERSION);

    signal(SIGPIPE, SIG_IGN);

    epoll_fd = initialize_pool();

    if(epoll_fd < 0)
    {
        return 1;
    }

    srvFD = start_server(80);

    if(srvFD < 0)
    {
        return 1;
    }

    acc_str = xmalloc(sizeof(struct handler_prm_t));
    acc_str->sockFD = srvFD;
    acc_str->fileFD = -1;
    acc_str->epoll_fd = epoll_fd;

    acc_str->buffer_malloced = 0;
    acc_str->buffer = NULL;
    acc_str->buf_offset = 0;
    acc_str->buf_len = 0;

    acc_str->critical = 1;
    acc_str->has_expiration = 0;
    acc_str->expiration_date = 0;

    acc_str->request = NULL;

    acc_str->processor = handle_http_accept;

    ret = submit_to_pool(epoll_fd, acc_str);

    if(ret < 0)
    {
        return 1;
    }

    start_pool_threads(7, &epoll_fd);
    pool_worker(&epoll_fd);

    return 0;
}