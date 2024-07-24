#include <string.h>
#include "../sys.h"

int cgi_value(char *opt, char *name, char *value, int size) {
	char	*ptr, *cmp, *dest, hex[3];
	int	c, flag, code;

	ptr = &opt[-1];
	value[0] = 0;
	hex[2] = 0;
	do {
		ptr++;
		cmp = name;
		flag = 1;
		while(*ptr != '=' && *ptr != '&' && *ptr != 0) {
			if(*ptr++ != *cmp++) {
				flag = 0;
				break;
			}
		}
		if(*ptr++ != '=') flag = 0;
		if(flag == 1) {
			dest = value;
			c = 0;
			while(*ptr != '&' && *ptr != 0 && c++ < size) {
				if(*ptr == '%') {
					ptr++;
					hex[0] = *ptr++;
					hex[1] = *ptr++;
					sscanf(hex, "%x", &code);
					*dest++ = code;
				} else if(*ptr == '+') {
					ptr++;
					*dest++ = ' ';
				} else {
					*dest++ = *ptr++;
				}
			}
			*dest = 0;
			return 0;
		}
		ptr = strchr(ptr, '&');
	} while(ptr != 0);
	return -1;
}
