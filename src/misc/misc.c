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

int string_starts_with(const char *haystack, const char *needle)
{
    if(memcmp(haystack, needle, strlen(needle)))
    {
        return 1;
    }

    return 0;
}

int string_ends_with(const char *haystack, const char *needle)
{
    int hlen = strlen(haystack);
    int nlen = strlen(needle);

    if(hlen < nlen)
    {
        return 1;
    }

    if(memcmp(haystack + hlen - nlen, needle, nlen))
    {
        return 1;
    }

    return 0;
}