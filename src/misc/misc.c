#include "../main.h"

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