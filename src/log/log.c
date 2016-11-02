#include "../main.h"

static int log_fd = STDOUT_FILENO;

int logInit(const char *log_file)
{
    if(log_file == NULL)
    {
        log_fd = STDOUT_FILENO;

        return 0;
    }

    log_fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT);

    if(log_fd < 0)
    {
        perror("can not initialize log file");

        log_fd = STDOUT_FILENO;

        return -1;
    }

    return 0;
}

void logWrite(enum log_type_t type, const char *template, const unsigned int n, ...)
{
    va_list args;

    int i;

    char buffer[1024];

    time_t now;

    struct tm *now_tm;

    time(&now);
    now_tm = gmtime(&now);

	i = strftime(buffer, 100,"[%d/%b/%Y:%H:%M:%S]", now_tm);
    
    switch(type)
    {
        case LOG_TYPE_INFO:
        {
            i += sprintf(buffer + i, "[INFO]");
        }
        break;

        case LOG_TYPE_ERROR:
        {
            i += sprintf(buffer + i, "[ERROR]");
        }
        break;

        case LOG_TYPE_FATAL:
        {
            i += sprintf(buffer + i, "[FATAL]");
        }
        break;
    }

    va_start(args, n);
    i += vsnprintf(buffer + i, 900, template, args);
    va_end(args);

    buffer[i] = '\n';

    write(log_fd, buffer, i + 1);
}