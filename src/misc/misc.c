#include "../main.h"

time_t _utcTime()
{
    time_t now;

    now = time(NULL);
    now = timegm(gmtime(&now));

    return now;
}

void *xmalloc(const unsigned long int size)
{
	void *ret;

	ret = malloc(size);

	if(ret == NULL)
	{
		logWrite(LOG_TYPE_FATAL, "out of memory", 0);
		
		exit(EXIT_FAILURE);
	}

	return ret;
}