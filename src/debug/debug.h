#ifndef _DEBUG_H_
#define _DEBUG_H_

//#define BUG_CHECK(X)

#ifndef BUG_CHECK
#define BUG_CHECK(X) \
    if(X) \
    { \
        logWrite(LOG_TYPE_FATAL, "BUG DETECTED in file %s, line %d", 2, __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    }
#endif

#endif