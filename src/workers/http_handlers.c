#include "../main.h"

void handle_http_receive(void *arg)
{
    struct handler_prm_t *prm = arg;

    int readRet;

    readRet = read(prm->sockFD, prm->buffer + prm->buf_offset, prm->buf_len - prm->buf_offset);

    if(readRet <= 0)
    {
        return;
    }

    prm->buf_offset += readRet;
    prm->buffer[prm->buf_offset] = '\0';

    logWrite(LOG_TYPE_INFO, "read %s", 1, prm->buffer);
}

void handle_http_accept(void *arg)
{
    struct handler_prm_t *prm = arg;
    struct handler_prm_t *cli_prm;

    unsigned char ip[16];

    socklen_t ip_len = 16;

    int client;

    client = accept_conection(prm->sockFD, ip, &ip_len);

    if(client < 0)
    {
        return;
    }

    logWrite(LOG_TYPE_INFO, "got connection %d", 1, client);

    BUG_CHECK(ip_len != 4 && ip_len != 16);

    if(ip_len == 4)
    {
        logWrite(LOG_TYPE_INFO, "IPv4: " IPV4_PRINT_TEMPLATE, 4, IPV4_PRINT(ip));
    }

    if(ip_len == 16)
    {
        logWrite(LOG_TYPE_INFO, "IPv6: " IPV6_PRINT_TEMPLATE, 16, IPV6_PRINT(ip));
    }

    cli_prm = xmalloc(sizeof(struct handler_prm_t));
    cli_prm->sockFD = client;
    cli_prm->epoll_fd = prm->epoll_fd;

    cli_prm->buffer_malloced = 1;
    cli_prm->buffer = xmalloc(2048);
    cli_prm->buf_len = 2048;
    cli_prm->buf_offset = 0;

    cli_prm->critical = 0;
    cli_prm->has_expiration = 1;
    cli_prm->expiration_date = _utcTime() + 5;

    cli_prm->processor = handle_http_receive;

    if(submit_to_pool(cli_prm->epoll_fd, cli_prm))
    {
        close(client);

        free(cli_prm->buffer);
        free(cli_prm);
    }
}