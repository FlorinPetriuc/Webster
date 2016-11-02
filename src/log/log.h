#ifndef _LOG_H_
#define _LOG_H_

enum log_type_t
{
    LOG_TYPE_INFO,
    LOG_TYPE_ERROR,
    LOG_TYPE_FATAL
};

int logInit(const char *log_file);

void logWrite(enum log_type_t type, const char *template, const unsigned int n, ...);

#endif