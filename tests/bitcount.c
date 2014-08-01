#include <stdint.h>
unsigned
bitcount(uint64_t u)
{
	int c = 0;
	for (; u > 0; u &= u - 1)
		c++;
	return c;
}
