#ifndef _MISC_H_
#define _MISC_H_

#define INIT_MUTEX(x) \
        if(pthread_mutex_init(&x, NULL) != 0) \
        { \
            exit(EXIT_FAILURE); \
        }

#define TAKE_MUTEX(x) \
        if(pthread_mutex_lock(&x) != 0) \
        { \
            exit(EXIT_FAILURE); \
        }

#define RELEASE_MUTEX(x) \
        if(pthread_mutex_unlock(&x) != 0) \
        { \
            exit(EXIT_FAILURE); \
        }

#define MACRO_STRLEN(x) (sizeof(x) - 1)

#define IPV4_PRINT_TEMPLATE "%hhu.%hhu.%hhu.%hhu"
#define IPV4_PRINT(X)       (X)[3], (X)[2], (X)[1], (X)[0]
#define IPV6_PRINT_TEMPLATE "%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx"
#define IPV6_PRINT(X)       (X)[15], (X)[14], (X)[13], (X)[12], (X)[11], (X)[10], (X)[9], (X)[8], (X)[7], (X)[6], (X)[5], (X)[4], (X)[3], (X)[2], (X)[1], (X)[0]

time_t _utcTime();
void *xmalloc(const unsigned long int size);

int string_starts_with(const char *haystack, const char *needle);
int string_ends_with(const char *haystack, const char *needle);

#endif