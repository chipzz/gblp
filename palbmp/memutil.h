#include <stdlib.h>
#define new(dtype) calloc(1,sizeof(dtype))
#define smartfree(m) if ( (m) == NULL ) ; else { free((void *)m); m = NULL; }
#define destroy(m) smartfree(m)
#define memclear(a,b) 	memset(a, '\0', b)
