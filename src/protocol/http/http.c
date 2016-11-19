#include "../../main.h"

static const char *working_directory = ".";
static int working_directory_len = 1;

void http_set_working_directory(const char *path)
{
    if(path == NULL)
    {
        working_directory = ".";
        working_directory_len = 1;

        logWrite(LOG_TYPE_INFO, "Working directory is %s", 1, working_directory);

        return;
    }

    working_directory = path;
    working_directory_len = strlen(working_directory);

    logWrite(LOG_TYPE_INFO, "Working directory is %s", 1, working_directory);
}

static char *http_resolve_path_and_version(const char *header, unsigned char *version_major,
                                                                unsigned char *version_minor)
{
    unsigned int i = 0;
    unsigned int j = 0;

    unsigned int length;

    int depth = 0;

    char *ret;

    //GET <path> HTTP/X.X
    while(header[i] != ' ')
    {
        if(header[i] == '\0')
        {
            return NULL;
        }

        ++i;
    }

    ++i;
    j = i;

    //<path> HTTP/X.X
    while(header[j] != ' ')
    {
        if(header[j] == '\0')
        {
            return NULL;
        }

        if(header[j] == '/')
        {
            ++depth;

             ++j;

            if(header[j] == '/')
            {
                return NULL;
            }

            continue;
        }

        if(header[j] != '.')
        {
            ++j;

            continue;
        }

        ++j;

        if(header[j] != '.')
        {
            continue;
        }

        depth -= 2;

        if(depth < 0)
        {
            return NULL;
        }

        ++j;
    }

    if(sscanf(header + j + 1, "HTTP/%hhu.%hhu", version_major, version_minor) != 2)
    {
        return NULL;
    }

    length = j - i;

    ret = xmalloc(working_directory_len + length + 1);
    memcpy(ret, working_directory, working_directory_len);

    j = working_directory_len;

    length += i;

    while(i < length)
    {
        if(header[i] != '%')
        {
            ret[j] = header[i];

            ++j;
            ++i;

            continue;
        }

        ++i;

        if(header[i] == '%')
        {
            ret[j] = '%';

            ++j;
            ++i;

            continue;
        }

        if(header[i] >= '0' && header[i] <= '9')
        {
            ret[j] = (header[i] - '0') << 4;
        }
        else if(header[i] >= 'a' && header[i] <= 'f')
        {
            ret[j] = (header[i] - 'a' + 10) << 4;
        }
        else if(header[i] >= 'A' && header[i] <= 'F')
        {
            ret[j] = (header[i] - 'A' + 10) << 4;
        }

        ++i;

        if(header[i] >= '0' && header[i] <= '9')
        {
            ret[j] += header[i] - '0';
        }
        else if(header[i] >= 'a' && header[i] <= 'f')
        {
            ret[j] += header[i] - 'a' + 10;
        }
        else if(header[i] >= 'A' && header[i] <= 'F')
        {
            ret[j] += header[i] - 'A' + 10;
        }

        ++j;
        ++i;
    }

    ret[j] = '\0';

    return ret;
}

struct http_request_t *parse_http_header(const char *header)
{
    struct http_request_t *ret;

    char *path;

    unsigned char version_major = 0;
    unsigned char version_minor = 0;

    if(string_starts_with(header, "GET "))
    {
        logWrite(LOG_TYPE_ERROR, "Only get requests are supported in version %u", 1, WEBSTER_VERSION);

        return NULL;
    }

    path = http_resolve_path_and_version(header, &version_major, &version_minor);

    if(path == NULL)
    {
        logWrite(LOG_TYPE_ERROR, "Could not solve http path or version", 0);

        return NULL;
    }

    ret = xmalloc(sizeof(struct http_request_t));
    ret->req_type = HTTP_GET;
    ret->abs_path = path;
    ret->version_major = version_major;
    ret->version_minor = version_minor;

    logWrite(LOG_TYPE_INFO, "Header resolved to path %s, version: %hhu.%hhu", 3,
                                                path, version_major, version_minor);

    return ret;
}