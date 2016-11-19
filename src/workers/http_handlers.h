#ifndef _HTTP_HANDLERS_H_
#define _HTTP_HANDLERS_H_

int handle_http_receive(void *arg);
int handle_http_accept(void *arg);
int handle_http_send_page(void *arg);
int handle_http_send_bad_request(void *arg);
int handle_http_send_not_found(void *arg);
int handle_http_send_server_error(void *arg);

#endif