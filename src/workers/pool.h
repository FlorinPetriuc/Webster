#ifndef _POOL_H_
#define _POOL_H_

int initialize_pool();
int submit_to_pool(const int epoll_fd, struct handler_prm_t *prm);

void *pool_worker(void *arg);
void start_pool_threads(const unsigned int thread_count, void *arg);

#endif