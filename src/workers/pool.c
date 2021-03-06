/*
 * Copyright (C) 2016 Florin Petriuc. All rights reserved.
 * Initial release: Florin Petriuc <petriuc.florin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "../main.h"

int initialize_pool()
{
    int ret;

    ret = epoll_create1(0);

    if(ret < 0)
    {
        logWrite(LOG_TYPE_FATAL, "epoll_create1 failed %s", 1, strerror(errno));

        return -1;
    }

    return ret;
}

int submit_to_pool(const int epoll_fd, struct handler_prm_t *prm)
{
    int ret;

    struct epoll_event evt;

    evt.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLPRI | EPOLLET | EPOLLONESHOT;
    evt.data.ptr = prm;

    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, prm->sockFD, &evt);

    if(ret < 0)
    {
        logWrite(LOG_TYPE_ERROR, "epoll_ctl failed %s", 1, strerror(errno));

        return -1;
    }

    return 0;
}

void resubmit_to_pool(const int epoll_fd, struct epoll_event *evt)
{
    int ret;

    struct handler_prm_t *prm;

    evt->events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLPRI | EPOLLET | EPOLLONESHOT;

    prm = evt->data.ptr;

    ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, prm->sockFD, evt);

    if(ret < 0)
    {
        logWrite(LOG_TYPE_FATAL, "epoll_ctl failed %s", 1, strerror(errno));

        exit(EXIT_FAILURE);
    }
}

void remove_from_pool(const int epoll_fd, struct handler_prm_t *prm)
{
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, prm->sockFD, NULL))
    {
        logWrite(LOG_TYPE_FATAL, "epoll_ctl failed %s", 1, strerror(errno));

        exit(EXIT_FAILURE);
    }

    if(prm->ssl)
    {
        close_secure_connection(prm->ssl);
    }
    else
    {
        close_connection(prm->sockFD);
    }

    if(prm->fileFD != -1)
    {
        close(prm->fileFD);
    }

    if(prm->client_h2settings)
    {
        free(prm->client_h2settings);
    }
    
    if(prm->critical)
    {
        logWrite(LOG_TYPE_FATAL, "critical socket has closed", 0);

        exit(EXIT_FAILURE);
    }

    if(prm->in_buffer)
    {
        free(prm->in_buffer);
    }

    if(prm->out_buffer)
    {
        free(prm->out_buffer);
    }

    if(prm->request)
    {
        free(prm->request->abs_path);
        free(prm->request);
    }

    free(prm);
}

void *pool_worker(void *arg)
{
    int epoll_fd = *(int *)arg;
    int wRet;
    int i;
    
    struct handler_prm_t *prm;
    struct epoll_event evts[10];

    time_t now;

    while(1)
    {
        wRet = epoll_wait(epoll_fd, evts, 10, -1);

        if(wRet <= 0)
        {
            logWrite(LOG_TYPE_FATAL, "epoll_wait failed %s", 1, strerror(errno));

            exit(EXIT_FAILURE);
        }

        for(i = 0; i < wRet; ++i)
        {
            prm = evts[i].data.ptr;

            if((evts[i].events & EPOLLRDHUP) ||
               (evts[i].events & EPOLLERR) ||
               (evts[i].events & EPOLLHUP))
            {
                remove_from_pool(epoll_fd, prm);

                continue;
            }

            if(prm->has_expiration == 0)
            {
                if(prm->processor(prm))
                {
                    remove_from_pool(epoll_fd, prm);
                }
                else
                {
                    resubmit_to_pool(epoll_fd, &evts[i]);
                }

                continue;
            }

            now = _utcTime();

            if(now >= prm->expiration_date)
            {
                logWrite(LOG_TYPE_INFO, "socket %d timed out", 1, prm->sockFD);

                remove_from_pool(epoll_fd, prm);

                continue;
            }

            if(prm->processor(prm))
            {
                remove_from_pool(epoll_fd, prm);
            }
            else
            {
                resubmit_to_pool(epoll_fd, &evts[i]);
            }
        }
    }

    return NULL;
}

void start_pool_threads(const unsigned int thread_count, void *arg)
{
    unsigned int i;

    pthread_t t;

    for(i = 0; i < thread_count; ++i)
    {
        if(pthread_create(&t, NULL, pool_worker, arg))
        {
            logWrite(LOG_TYPE_FATAL, "can not start workers: %s", 1, strerror(errno));

            exit(EXIT_FAILURE);
        }
    }
}