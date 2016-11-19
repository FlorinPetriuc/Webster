#ifndef _HTTP_H_
#define _HTTP_H_

enum http_request_type_t
{
    HTTP_GET,
    HTTP_POST
};

struct http_request_t
{
    enum http_request_type_t req_type;

    char *abs_path;

    unsigned version_major;
    unsigned version_minor;
};

struct http_request_t *parse_http_header(const char *header);
void http_set_working_directory(const char *path);

#endif