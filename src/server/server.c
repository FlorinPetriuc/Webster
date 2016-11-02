#include "../main.h"

int start_server(const unsigned short int port)
{
    int fd;
    int reuse = 1;

    struct sockaddr_in6 serv_addr;

    fd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    
    if(fd < 0)
    {
        return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(port);

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
    {
        logWrite(LOG_TYPE_FATAL, "unable to reuse server socket %s", 1, strerror(errno));

        close(fd);

        return -1;
    }

    if(bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        logWrite(LOG_TYPE_FATAL, "unable to bind server socket %s", 1, strerror(errno));

        close(fd);

        return -1;
	}

    if(listen(fd, 100) < 0)
    {
        logWrite(LOG_TYPE_FATAL, "unable to listen server socket %s", 1, strerror(errno));

        close(fd);

		return -1;
	}
    
    return fd;
}

int accept_conection(const int server_socket, unsigned char *ip, socklen_t *ip_len)
{
    struct sockaddr_in *cli_addrV4;
    struct sockaddr_in6 *cli_addrV6;
    struct sockaddr_storage cli_addr;

    int ret;
    int flags;

    memset(ip, 0, *ip_len);

    *ip_len = sizeof(struct sockaddr_storage);

    ret = accept(server_socket, (struct sockaddr *)&cli_addr, ip_len);

    if(ret < 0)
    {
        return -1;
    }

    switch(cli_addr.ss_family)
    {
        case AF_INET:
        {
            *ip_len = 4;

            cli_addrV4 = (struct sockaddr_in *)&cli_addr;

            memcpy(ip, &cli_addrV4->sin_addr.s_addr, 4);
        }
        break;

        case AF_INET6:
        {
            *ip_len = 16;

            cli_addrV6 = (struct sockaddr_in6 *)&cli_addr;

            memcpy(ip, cli_addrV6->sin6_addr.s6_addr, 16);
        }
        break;

        default:
        {
            logWrite(LOG_TYPE_ERROR, "wrong sa_family accepted %hhd", 1, cli_addr.ss_family);

            close(ret);
        }
        return -1;
    }

    flags = fcntl(ret, F_GETFL, 0);

    if(flags < 0)
    {
        logWrite(LOG_TYPE_ERROR, "fcntl F_GETFL failed on socket %d", 1, ret);

        close(ret);

        return -1;
    }
   
    flags |= O_NONBLOCK;
   
    if(fcntl(ret, F_SETFL, flags))
    {
        logWrite(LOG_TYPE_ERROR, "fcntl F_SETFL failed on socket %d", 1, ret);

        close(ret);

        return -1;
    }

    return ret;
}

void close_connection(const int client_socket)
{
    close(client_socket);
}